/**
 * @file HttpManager.hpp
 * @brief Manages HTTP request processing and response generation.
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-13
 * @version 1.0
 *
 * The HttpManager class coordinates HTTP request handling and response
 * creation.
 *
 * @todo
 * 1) Double validation of request:
 *    GET:
 *      - should not have body -> 4xx client error
 * 2) Should we handle http://... and https://... uri?
 * 3) Handle error pages from config
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
class HttpRequestParser;
class Logger;

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
  HttpRequestParser _parser;
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
  void redurectToDefaultCatErrorPage(int status_code, HttpResponse& response);
};

#endif  // _HTTP_MANAGER_HPP
