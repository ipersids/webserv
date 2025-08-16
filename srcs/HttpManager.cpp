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

    : _config(config), _parser() {(void) _config;}

std::string HttpManager::processHttpRequest(const std::string& raw_request,
                                            HttpRequest& request,
                                            HttpResponse& response) {
  int exit_code = _parser.parseRequest(raw_request, request);
  setConnectionHeader(request, response);
  if (exit_code != 0) {
    std::string msg(request.getStatus().message);
    response.setErrorResponse(request.getStatus().status_code, msg);
    logError(request, "Failed to parse request: " + msg);
    return response.convertToString();
  }

  logInfo("Processing " + request.getMethod() +
          " request: " + request.getRequestTarget());

  /// @todo handle GET POST and DELETE

  /// start @test >>>
  response.setBody("<h1>Hello from webserv!</h1>");
  response.insertHeader("Content-Type", "text/html");
  /// end @ test <<<

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

void HttpManager::setConnectionHeader(const HttpRequest& request,
                                      HttpResponse& response) {
  if (response.hasHeader("Connection")) {
    response.removeHeader("Connection");
  }

  std::string conn = HttpUtils::toLowerCase(request.getHeader("Connection"));
  std::string version = request.getHttpVersion();
  HttpUtils::HttpStatusCode code = request.getStatus().status_code;

  if (code == HttpUtils::HttpStatusCode::BAD_REQUEST ||
      code == HttpUtils::HttpStatusCode::REQUEST_TIMEOUT ||
      code >= HttpUtils::HttpStatusCode::LENGTH_REQUIRED || conn == "close" ||
      (version == "HTTP/1.0" && conn != "keep-alive")) {
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
