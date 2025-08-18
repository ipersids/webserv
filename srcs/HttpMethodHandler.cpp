/**
 * @file HttpMethodHandler.cpp
 * @brief Handles HTTP method processing for web server requests
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-14
 * @version 1.0
 *
 */

#include "HttpMethodHandler.hpp"

// constructor

/**
 * @brief Constructs an HttpMethodHandler with the given server configuration
 * @param config Reference to the server configuration object
 *
 * @note The configuration reference must remain valid for the lifetime
 *       of this HttpMethodHandler instance.
 */
HttpMethodHandler::HttpMethodHandler(const ConfigParser::ServerConfig& config)
    : _config(config) {}

// public methods

/**
 * @brief Processes an HTTP request and returns the appropriate response
 *
 * This is the main entry point for HTTP method processing. It analyzes the
 * request method and routes it to the appropriate handler (GET, POST, DELETE).
 * The method also validates permissions and applies location-specific rules.
 *
 * @param request The HTTP request object containing method, URI, headers, and
 * body
 * @return HttpResponse object containing the complete response
 */
HttpResponse HttpMethodHandler::processMethod(const HttpRequest& request) {
  HttpResponse response;
  const std::string uri = request.getRequestTarget();

  // find the longest match of location (config) for given target URI
  const ConfigParser::LocationConfig* location =
      HttpUtils::getLocation(uri, _config);
  if (!location) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Requested location not found");
    Logger::error("Requested location not found: " + uri);
    return response;
  }

  // check if this location requires redirection to another one
  if (!location->redirect_url.empty()) {
    response.setStatusCode(HttpUtils::HttpStatusCode::MOVED_PERMANENTLY);
    response.setBody("Redirecting to " + location->redirect_url);
    response.insertHeader("Location", location->redirect_url);
    response.insertHeader("Content-Type", "text/html");
    Logger::warning("Redirecting " + uri + " to " + location->redirect_url);
    return response;
  }

  // find full path to requested target URI
  const std::string file_path = HttpUtils::getFilePath(*location, uri);
  std::string message = "";
  if (!HttpUtils::isFilePathSecure(file_path, location->root, message)) {
    Logger::warning("Possible security problem " + uri);
    Logger::error("Failed to resolve uri " + uri + ": " + message);
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Page/file doesn't exist");
    return response;
  }

  // check if method is allowed by location in config file
  if (!HttpUtils::isMethodAllowed(*location, request.getMethod())) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::METHOD_NOT_ALLOWED,
                              "Method " + request.getMethod() + " not allowed");
    return response;
  }

  // perform method GET, POST or DELETE or give error
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

/// protected methods

/**
 * @brief Handles HTTP GET requests
 *
 * Processes GET requests by serving static files or generating directory
 * listings based on the requested path and location configuration.
 *
 * @param path The file system path to the requested resource
 * @param uri The original URI from the request
 * @param location The location configuration block that matches this request
 *
 * @return HttpResponse containing the file content or directory listing
 *
 * @see serveStaticFile()
 * @see serveDirectoryContent()
 */
HttpResponse HttpMethodHandler::handleGetMethod(
    const std::string& path, const std::string& uri,
    const ConfigParser::LocationConfig& location) {
  HttpResponse response;

  // check if file/directory exists
  if (!std::filesystem::exists(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "File not found");
    Logger::error("Requested file doesn't exist: " + path);
    return response;
  }

  // check if it's a directory
  if (std::filesystem::is_directory(path)) {
    // check if we have default file to serve
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

    // check if directory listing is allowed by config
    if (location.autoindex) {
      Logger::info("Listing directory: " + path);
      return serveDirectoryContent(path, uri);
    }

    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied");
    Logger::error("Directory access denied: " + path);
    return response;
  }

  // handle requested file
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

/// @warning IT IS PLACEHOLDER, implementation needed
HttpResponse HttpMethodHandler::handlePostMethod(const std::string& path) {
  HttpResponse response;
  response.setBody("<h1>Hello from webserv!</h1>");
  response.insertHeader("Content-Type", "text/html");
  (void)path;
  return response;
}

/// @warning IT IS PLACEHOLDER, implementation needed
HttpResponse HttpMethodHandler::handleDeleteMethod(const std::string& path) {
  HttpResponse response;
  response.setBody("<h1>Hello from webserv!</h1>");
  response.insertHeader("Content-Type", "text/html");
  (void)path;
  return response;
}

/// Helper functions

/**
 * @brief Serves a static file from the file system
 *
 * Reads a file from the file system and creates an HTTP response with
 * appropriate headers including Content-Type, Content-Length, and caching
 * headers.
 *
 * @param path The file system path to the file to serve
 * @return HttpResponse containing the file content and appropriate headers
 */
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

/**
 * @brief Generates and serves directory listing content
 *
 * Creates an HTML directory listing page showing files and subdirectories
 * in the specified path. Only used when auto-index is enabled in the
 * location configuration.
 *
 * @param path The file system path to the directory to list
 * @param uri The original URI from the request (used for link generation)
 * @return HttpResponse containing HTML directory listing
 */
HttpResponse HttpMethodHandler::serveDirectoryContent(const std::string& path,
                                                      const std::string& uri) {
  HttpResponse response;

  try {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
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
