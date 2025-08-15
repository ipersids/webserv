/**
 * @file HttpManager.cpp
 * @brief Manages HTTP request processing and response generation.
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-13
 * @version 1.0
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

HttpManager::HttpManager(const ConfigParser::ServerConfig& config)
    : _config(config), _parser(), _method_handler(config) {
  (void)_config;
}

std::string HttpManager::processHttpRequest(const std::string& raw_request,
                                            HttpRequest& request,
                                            HttpResponse& response) {
  int exit_code = _parser.parseRequest(raw_request, request);
  if (exit_code != 0) {
    response = buildErrorResponse(request, request.getStatus().status_code,
                                  request.getStatus().message,
                                  "Failed to parse request");
    return response.convertToString();
  }

  logInfo("Processing " + request.getMethod() +
          " request: " + request.getRequestTarget());

  response = _method_handler.processMethod(request);
  setConnectionHeader(request.getHeader("Connection"), request.getHttpVersion(),
                      response);

  return response.convertToString();
}

bool HttpManager::keepConnectionAlive(const HttpResponse& response) {
  std::string response_connection =
      HttpUtils::toLowerCase(response.getHeader("Connection"));

  if (response_connection == "close") {
    return false;
  }

  return true;
}

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

void HttpManager::logRequestInfo(const HttpRequest& request) {
  std::ostringstream log_message;

  log_message << "\"" << request.getMethod() << " "
              << request.getRequestTarget() << " " << request.getHttpVersion()
              << "\"" << static_cast<int>(request.getStatus().status_code);

  Logger::info(log_message.str());
}

void HttpManager::logError(const HttpRequest& request, const std::string& msg) {
  Logger::error("HttpManager: " + msg);
  logRequestInfo(request);
}

void HttpManager::logInfo(const std::string& msg) {
  Logger::info("HttpManager: " + msg);
}
