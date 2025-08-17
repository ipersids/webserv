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

  const ConfigParser::LocationConfig* location =
      HttpUtils::getLocation(uri, _config);
  if (!location) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Page/file doesn't exist");
    Logger::error("Requested location not found: " + uri);
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

  const std::string file_path = HttpUtils::getFilePath(*location, uri);
  std::string message = "";
  if (!HttpUtils::isFilePathSecure(file_path, location->root, message)) {
    Logger::warning("Possibly a security problem " + uri);
    Logger::error("Failed resolve uri " + uri + ": " + message);
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Page/file doesn't exist");
    return response;
  }

  if (!HttpUtils::isMethodAllowed(*location, request.getMethod())) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::METHOD_NOT_ALLOWED,
                              "Method " + request.getMethod() + " not allowed");
    return response;
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
          "Method " + request.getMethod() + " not implemented");
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

HttpResponse HttpMethodHandler::serveStaticFile(const std::string& path) {
  HttpResponse response;
  std::string body = "";

  if (HttpUtils::getFileContent(path, body) == -1) {
    Logger::error(body + ": " + path);
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied");
    return response;
  }
  response.setBody(body);
  response.setStatusCode(HttpUtils::HttpStatusCode::OK);
  response.insertHeader("Content-Type", HttpUtils::getMIME(path));
  Logger::info("Served file: " + path + " (" +
               response.getHeader("Content-Length") + " bytes)");
  return response;
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
