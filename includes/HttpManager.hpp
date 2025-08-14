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
 */

#ifndef _HTTP_MANAGER_HPP
#define _HTTP_MANAGER_HPP

#include <string>

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

 private:
  const ConfigParser::ServerConfig& _config;
  HttpRequestParser _parser;

 private:
  void logRequestInfo(const HttpRequest& request);
  void logError(const HttpRequest& request, const std::string& msg);
  void logInfo(const std::string& msg);
  void setConnectionHeader(const HttpRequest& request, HttpResponse& response);
};

#endif  // _HTTP_MANAGER_HPP
