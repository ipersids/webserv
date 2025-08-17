/**
 * @file HttpMethodHandler.hpp
 * @brief Handles HTTP method processing for webserver requests
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-14
 * @version 1.0
 *
 * @see GET request method:
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/GET
 *
 */

#ifndef _HTTP_METHOD_HANDLER_HPP
#define _HTTP_METHOD_HANDLER_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "config.hpp"

class HttpRequest;
class HttpResponse;

class HttpMethodHandler {
 public:
  HttpMethodHandler() = delete;
  HttpMethodHandler& operator=(const HttpMethodHandler& other) = delete;
  HttpMethodHandler(const HttpMethodHandler& other) = delete;
  ~HttpMethodHandler() = default;
  HttpMethodHandler(const ConfigParser::ServerConfig& config);

  HttpResponse processMethod(const HttpRequest& request);

 private:
  // variables
  const ConfigParser::ServerConfig& _config;

 protected:
  // main functions
  HttpResponse handleGetMethod(const std::string& path, const std::string& uri,
                               const ConfigParser::LocationConfig& location);
  HttpResponse handlePostMethod(const std::string& path);
  HttpResponse handleDeleteMethod(const std::string& path);

 protected:
  // helper functions
  HttpResponse serveStaticFile(const std::string& path);
  HttpResponse serveDirectoryContent(const std::string& path,
                                     const std::string& uri);
};

#endif  /// _HTTP_METHOD_HANDLER_HPP
