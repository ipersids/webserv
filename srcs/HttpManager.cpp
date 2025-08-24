/**
 * @file HttpManager.cpp
 * @brief Manages HTTP request processing and response generation.
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-22
 * @version 2.0
 *
 * The HttpManager class coordinates HTTP request handling and response
 * creation.
 *
 * @see
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Connection_management_in_HTTP_1.x
 * Connection management in HTTP/1.x
 *
 * @todo
 * - keep-alive timeout from config file (default 75s)
 *   https://nginx.org/en/docs/http/ngx_http_core_module.html#keepalive_timeout
 *
 */

#include "HttpManager.hpp"

/// constructor
/**
 * @brief Constructs an HttpManager with the given server configuration
 * @param config Reference to the server configuration object
 * @note The configuration reference must remain valid for the lifetime
 *       of this HttpManager instance.
 */
HttpManager::HttpManager(const ConfigParser::ServerConfig& config)
    : _config(config), _method_handler(config) {
  (void)_config;
}

/// public methods

/**
 * @brief Processes a raw HTTP request and generates a response
 * @param raw_request The raw HTTP request string received from the client
 * @param request HttpRequest object to be populated with parsed data
 * @param response HttpResponse object to be populated with response data
 * @return String representation of the complete HTTP response ready to send
 *
 * @note The request and response objects are modified by this method
 */
std::string HttpManager::processHttpRequest(const std::string& raw_request,
                                            HttpRequest& request,
                                            HttpResponse& response) {
  // parse raw request into HttpRequest object
  HttpRequestParser::Status state =
      HttpRequestParser::parseRequest(raw_request, request);
  if (state == HttpRequestParser::Status::ERROR) {
    request.setParsingState(HttpParsingState::COMPLETE);
    response = buildErrorResponse(request, request.getStatusCode(),
                                  request.getErrorMessage(),
                                  "Failed to parse request");
    return response.convertToString();
  }

  logInfo("Processing " + request.getMethod() +
          " request: " + request.getRequestTarget());

  // execute method GET, POST or DELETE if allowed for given target uri
  response = _method_handler.processMethod(request);
  setConnectionHeader(request.getHeader("Connection"), request.getHttpVersion(),
                      response);

  int status_code = static_cast<int>(response.getStatusCode());
  if (status_code >= 400) {
    auto location = HttpUtils::getLocation(request.getRequestTarget(), _config);
    setErrorPageBody(location, status_code, response);
  }

  // return response as a string to send it to the client
  return response.convertToString();
}

/**
 * @brief Determines if the connection should be kept alive
 *
 * Analyzes the HTTP response to determine whether the connection should
 * remain open for additional requests or be closed after sending the response.
 * Decision is based on HTTP version, Connection header, and response status
 *
 * @param response The HTTP response object to analyze
 * @return true if connection should be kept alive, false otherwise
 */
bool HttpManager::keepConnectionAlive(const HttpResponse& response) {
  std::string response_connection =
      HttpUtils::toLowerCase(response.getHeader("Connection"));

  if (response_connection == "close") {
    return false;
  }

  return true;
}

/**
 * @brief Builds an error response for the given HTTP status code
 * @param request The original HTTP request that caused the error
 * @param code The HTTP status code for the error response
 * @param err_msg Error message to include in the response body
 * @param log_msg Message to log for debugging purposes
 *
 * @return Complete HttpResponse object for the error
 *
 * @note Logs the error using Logger class (see includes/Logger.hpp)
 */
HttpResponse HttpManager::buildErrorResponse(
    const HttpRequest& request, const HttpUtils::HttpStatusCode& code,
    const std::string& err_msg, const std::string& log_msg) {
  HttpResponse response;

  response.setErrorResponse(code, err_msg);
  setConnectionHeader(request.getHeader("Connection"), request.getHttpVersion(),
                      response);
  logError(request, log_msg + ": " + err_msg);

  return response;
}

// private methods

/**
 * @brief Sets the Connection header in the response
 * @param request_connection The Connection header value from the request
 * @param request_http_version The HTTP version from the request
 * @param response The response object to modify
 */
void HttpManager::setConnectionHeader(const std::string& request_connection,
                                      const std::string& request_http_version,
                                      HttpResponse& response) {
  if (response.hasHeader("Connection")) {
    return;
  }

  HttpUtils::HttpStatusCode code = response.getStatusCode();
  if (code == HttpUtils::HttpStatusCode::BAD_REQUEST ||
      code == HttpUtils::HttpStatusCode::REQUEST_TIMEOUT ||
      code >= HttpUtils::HttpStatusCode::LENGTH_REQUIRED ||
      request_connection == "close" ||
      (request_http_version == "HTTP/1.0" &&
       request_connection != "keep-alive")) {
    response.insertHeader("Connection", "close");
  } else {
    response.insertHeader("Connection", "keep-alive");
  }
}

/**
 * @brief Logs information about the incoming HTTP request
 * @param request The HTTP request to log information about
 */
void HttpManager::logRequestInfo(const HttpRequest& request) {
  std::ostringstream log_message;

  log_message << "\"" << request.getMethod() << " "
              << request.getRequestTarget() << " " << request.getHttpVersion()
              << "\"" << static_cast<int>(request.getStatusCode());

  Logger::info(log_message.str());
}

/**
 * @brief Logs an error message associated with a specific request
 * @param request The HTTP request associated with the error
 * @param msg The error message to log
 */
void HttpManager::logError(const HttpRequest& request, const std::string& msg) {
  Logger::error("HttpManager: " + msg);
  logRequestInfo(request);
}

/**
 * @brief Logs a general informational message
 * @param msg The informational message to log
 */
void HttpManager::logInfo(const std::string& msg) {
  Logger::info("HttpManager: " + msg);
}

/**
 * @brief Sets a custom error page body if configured
 * @param location Pointer to the location configuration (may be nullptr)
 * @param status_code The HTTP status code for the error
 * @param response The response object to modify
 */
void HttpManager::setErrorPageBody(const ConfigParser::LocationConfig* location,
                                   int status_code, HttpResponse& response) {
  std::string error_page_path = "/";
  std::map<int, std::string>::const_iterator it =
      location->error_pages.find(status_code);
  if (it != location->error_pages.end()) {
    error_page_path = it->second;
  } else {
    setDefaultCatErrorPage(status_code, response);
    Logger::error("Error page doesn't exist in config file: " +
                  std::to_string(status_code));
    Logger::warning("Getting default Cat error page '" +
                    std::to_string(status_code) + "'");
    return;
  }

  std::string path = HttpUtils::getFilePath(*location, error_page_path);
  std::string tmp_body = "";
  if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path) &&
      HttpUtils::getFileContent(path, tmp_body) == 0) {
    response.setBody(tmp_body);
    if (response.hasHeader("Content-Type")) {
      response.removeHeader("Content-Type");
    }
    response.insertHeader("Content-Type", HttpUtils::getMIME(path));
    Logger::info("Serve error page: " + path);
    return;
  }
  setDefaultCatErrorPage(status_code, response);
  Logger::error("Failed to get error page (" + std::to_string(status_code) +
                ") file: " + path);
  Logger::warning("Getting default Cat error page '" +
                  std::to_string(status_code) + "'");
}

/**
 * @brief Generates the default error page
 * @param status_code The HTTP status code for the error
 * @param response The response object to modify
 */
void HttpManager::setDefaultCatErrorPage(int status_code,
                                         HttpResponse& response) {
  std::string cat_url = "https://http.cat/" + std::to_string(status_code);
  std::string body =
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head><title>Error " +
      std::to_string(status_code) +
      "</title></head>\n"
      "<body style='text-align:center; font-family:Arial; "
      "background-color:black; color:white;'>\n"
      "<h1>Oooops! </h1>\n"
      "<p><a href='/' style='color:white;'>Go home!</a></p>\n"
      "<img src='" +
      cat_url + "' alt='HTTP Cat " + std::to_string(status_code) +
      "' style='max-width:100%; height:auto; margin:20px;'>\n"
      "</body></html>";

  response.setBody(body);
  if (response.hasHeader("Content-Type")) {
    response.removeHeader("Content-Type");
  }
  response.insertHeader("Content-Type", "text/html");
}
