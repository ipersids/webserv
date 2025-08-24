/**
 * @file HttpManager.hpp
 * @brief Manages HTTP request processing and response generation.
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-22
 * @version 2.0
 *
 * The HttpManager class coordinates HTTP request handling and response
 * creation.
 *
 * @todo
 * 1) Should we handle http://... and https://... uri?
 */

#ifndef _HTTP_MANAGER_HPP
#define _HTTP_MANAGER_HPP

#include <filesystem>
#include <string>

#include "HttpMethodHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include "HttpUtils.hpp"
#include "Logger.hpp"
#include "config.hpp"

class HttpResponse;
class HttpRequest;
class Logger;

/**
 * @class HttpManager
 * @brief Central HTTP request/response management class
 *
 * The HttpManager class serves as the main coordinator for HTTP request
 * processing and response generation in the Webserv.
 *
 * This class is responsible for:
 * - Processing raw HTTP requests
 * - Coordinating request parsing and validation
 * - Managing HTTP method handling (GET, POST, DELETE)
 * - Building appropriate HTTP responses
 * - Handling error responses and custom error pages
 * - Managing connection keep-alive logic
 */
class HttpManager {
 public:
  HttpManager(const ConfigParser::ServerConfig& config);
  ~HttpManager() = default;
  HttpManager() = delete;
  HttpManager(const HttpManager& other) = delete;
  HttpManager& operator=(const HttpManager& other) = delete;

  std::string processHttpRequest(const std::string& raw_request,
                                 HttpRequest& request, HttpResponse& response);
  bool keepConnectionAlive(const HttpResponse& response);
  HttpResponse buildErrorResponse(const HttpRequest& request,
                                  const HttpUtils::HttpStatusCode& code,
                                  const std::string& err_msg,
                                  const std::string& log_msg);

 private:
  const ConfigParser::ServerConfig& _config;
  HttpMethodHandler _method_handler;

 private:
  void logRequestInfo(const HttpRequest& request);
  void logError(const HttpRequest& request, const std::string& msg);
  void logInfo(const std::string& msg);
  void setConnectionHeader(const std::string& request_connection,
                           const std::string& request_http_version,
                           HttpResponse& response);
  void setErrorPageBody(const ConfigParser::LocationConfig* location,
                        int status_code, HttpResponse& response);
  void setDefaultCatErrorPage(int status_code, HttpResponse& response);
};

#endif  // _HTTP_MANAGER_HPP
