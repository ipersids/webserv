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
      response = handleGetMethod(file_path);
      break;
    case HttpMethod::POST:
      // response = handlePostMethod(file_path);
      break;
    case HttpMethod::DELETE:
      // response = handleDeleteMethod(file_path);
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

HttpResponse HttpMethodHandler::handleGetMethod(const std::string& path) {
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
