/**
 * @file HttpMethodHandler.cpp
 * @brief Handles HTTP method processing for webserver requests
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-14
 * @version 1.0
 *
 */

#include "HttpMethodHandler.hpp"

HttpMethodHandler::HttpMethodHandler(const ConfigParser::ServerConfig& config)
    : _config(config) {}

HttpResponse HttpMethodHandler::processMethod(const HttpRequest& request) {
  HttpResponse response;
  const std::string uri = request.getRequestTarget();

  const ConfigParser::LocationConfig* location = getLocation(uri);
  if (!location) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Page/file doesn't exist");
    Logger::error("Requested location not foun: " + uri);
    return response;
  }
  if (!location->redirect_url.empty()) {
    response.setStatusCode(HttpUtils::HttpStatusCode::MOVED_PERMANENTLY);
    response.setBody("Redirecting to " + location->redirect_url);
    response.insertHeader("Location", location->redirect_url);
    response.insertHeader("Content-Type", "text/html");
    Logger::warning("Redirecting " + uri + " to " + location->redirect_url);
    return response;
  }

  const std::string file_path = getFilePath(*location, uri);
  std::string message = "";
  if (!isFilePathSecure(file_path, location->root, message)) {
    Logger::warning("Possibly a security problem " + uri);
    Logger::error("Failed resolve uri " + uri + ": " + message);
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Page/file doesn't exist");
    return response;
  }

  if (!isMethodAllowed(*location, request.getMethod())) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::METHOD_NOT_ALLOWED,
                              "Method " + request.getMethod() + " not allowed");
  }

  const HttpMethod method_code = request.getMethodCode();
  switch (method_code) {
    case HttpMethod::GET:
      response = handleGetMethod(file_path, uri, *location);
      break;
    case HttpMethod::POST:
      response = handlePostMethod(file_path);
      break;
    case HttpMethod::DELETE:
      response = handleDeleteMethod(file_path);
      break;

    default:
      response.setErrorResponse(
          HttpUtils::HttpStatusCode::NOT_IMPLEMENTED,
          "Methog " + request.getMethod() + " not implemented");
      break;
  }

  return response;
}

/// Execute method

HttpResponse HttpMethodHandler::handleGetMethod(
    const std::string& path, const std::string& uri,
    const ConfigParser::LocationConfig& location) {
  HttpResponse response;

  if (!std::filesystem::exists(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "File not found");
    Logger::error("Requested file doesn't exist: " + path);
    return response;
  }

  if (std::filesystem::is_directory(path)) {
    if (!location.index.empty()) {
      std::string index_path = path;
      if (!index_path.empty() && index_path.back() != '/') {
        index_path += '/';
      }
      index_path += location.index;
      if (std::filesystem::exists(index_path) &&
          std::filesystem::is_regular_file(index_path)) {
        Logger::info("Serving file: " + index_path);
        return serveStaticFile(index_path);
      }
    }

    if (location.autoindex) {
      Logger::info("Listing directory: " + path);
      return serveDirectoryContent(path, uri);
    }

    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied");
    Logger::error("Directory access denied: " + path);
    return response;
  }

  if (std::filesystem::is_regular_file(path)) {
    Logger::info("Serving file: " + path);
    response = serveStaticFile(path);
  } else {
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied");
    Logger::error("File access denied: " + path);
  }

  return response;
}

HttpResponse HttpMethodHandler::handlePostMethod(const std::string& path) {
  HttpResponse response;
  response.setBody("<h1>Hello from webserv!</h1>");
  response.insertHeader("Content-Type", "text/html");
  (void)path;
  return response;
}

HttpResponse HttpMethodHandler::handleDeleteMethod(const std::string& path) {
  HttpResponse response;
  response.setBody("<h1>Hello from webserv!</h1>");
  response.insertHeader("Content-Type", "text/html");
  (void)path;
  return response;
}

/// Helper functions

const ConfigParser::LocationConfig* HttpMethodHandler::getLocation(
    const std::string& request_uri) {
  const ConfigParser::LocationConfig* location = nullptr;
  size_t max_location_length = 0;

  for (const ConfigParser::LocationConfig& loc : _config.locations) {
    if (request_uri.find(loc.path) == 0 &&
        loc.path.length() > max_location_length) {
      location = &loc;
      max_location_length = loc.path.length();
    }
  }

  return location;
}

const std::string HttpMethodHandler::getFilePath(
    const ConfigParser::LocationConfig& location,
    const std::string& request_uri) {
  std::string path = location.root;

  if (!path.empty() && path.back() != '/') {
    path += '/';
  }

  size_t start = (request_uri[0] == '/') ? 1 : 0;
  size_t end = (request_uri.find('?') != std::string::npos)
                   ? request_uri.find('?')
                   : request_uri.length();
  if (!request_uri.empty() && request_uri[0] == '/') {
    path += request_uri.substr(1, end - start);
  }

  return path;
}

bool HttpMethodHandler::isFilePathSecure(const std::string& path,
                                         const std::string& root,
                                         std::string& message) {
  std::string secure_path;
  std::string secure_root;
  try {
    secure_path = std::filesystem::weakly_canonical(path);
  } catch (const std::exception& e) {
    message = e.what();
    return false;
  }
  try {
    secure_root = std::filesystem::weakly_canonical(root);
  } catch (const std::exception& e) {
    message = e.what();
    return false;
  }

  if (secure_path.find(secure_root) != 0) {
    message = "Security violation detected";
    return false;
  }

  return true;
}

bool HttpMethodHandler::isMethodAllowed(
    const ConfigParser::LocationConfig& location, const std::string& method) {
  if (location.allowed_methods.empty()) {
    return true;
  }

  return std::find(location.allowed_methods.begin(),
                   location.allowed_methods.end(),
                   method) != location.allowed_methods.end();
}

HttpResponse HttpMethodHandler::serveStaticFile(const std::string& path) {
  HttpResponse response;

  try {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
      Logger::error("Failed read file: " + path);
      response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                                "Access denied");
      return response;
    }
    std::string body((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    file.close();
    response.setBody(body);
    response.setStatusCode(HttpUtils::HttpStatusCode::OK);
    response.insertHeader("Content-Type", getMIME(path));
    Logger::info("Served file: " + path + " (" +
                 response.getHeader("Content-Length") + " bytes)");
    return response;
  } catch (const std::exception& e) {
    Logger::error("Error serving file " + path + ": " + e.what());
    response.setErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Internal server error");
    return response;
  }
}

const std::string HttpMethodHandler::getMIME(const std::string& path) {
  size_t dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return "application/octet-stream";
  }

  std::string extension = HttpUtils::toLowerCase(path.substr(dot_pos + 1));

  if (extension == "html" || extension == "htm") return "text/html";
  if (extension == "css") return "text/css";
  if (extension == "js") return "application/javascript";
  if (extension == "json") return "application/json";
  if (extension == "xml") return "application/xml";
  if (extension == "txt") return "text/plain";
  if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
  if (extension == "png") return "image/png";
  if (extension == "gif") return "image/gif";
  if (extension == "ico") return "image/x-icon";
  if (extension == "pdf") return "application/pdf";
  if (extension == "zip") return "application/zip";
  if (extension == "tar") return "application/x-tar";

  return "application/octet-stream";
}

HttpResponse HttpMethodHandler::serveDirectoryContent(const std::string& path,
                                                      const std::string& uri) {
  HttpResponse response;

  try {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n "
         << "<title>Index of " << uri << "</title>\n"
         << "</head>\n"
         << "<body>\n"
         << "<h1>Index of " << uri << "</h1>\n"
         << "<hr>\n"
         << "<ul>\n";

    if (uri != "/") {
      std::string parent_uri = uri;
      if (parent_uri.back() == '/') {
        parent_uri.pop_back();
      }
      size_t slash = parent_uri.find_last_of('/');
      if (slash != std::string::npos) {
        parent_uri = parent_uri.substr(0, slash + 1);
      } else {
        parent_uri = "/";
      }
      html << "<li><a href=\"" << parent_uri << "\">../</a></li>\n";
    }

    std::string link = uri;
    if (link.back() != '/') {
      link += '/';
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      std::string name = entry.path().filename().string();

      if (name[0] == '.') {
        continue;
      }

      if (entry.is_directory()) {
        html << "<li><a href=\"" << link << name << "/\">" << name
             << "/</a></li>\n";
      } else {
        html << "<li><a href=\"" << link << name << "\">" << name
             << "</a></li>\n";
      }
    }

    html << "</ul>\n"
         << "<hr>\n"
         << "<p><em>Hello from Webserv!</em></p>\n"
         << "</body>\n"
         << "</html>";

    response.setStatusCode(HttpUtils::HttpStatusCode::OK);
    response.insertHeader("Content-Type", "text/html");
    response.setBody(html.str());
    Logger::info("Generated directory listing for: " + path);
    return response;

  } catch (const std::exception& e) {
    Logger::error("Error listing directory " + path + ": " + e.what());
    response.setErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Internal server error");
  }

  return response;
}
