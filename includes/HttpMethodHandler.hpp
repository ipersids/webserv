/**
 * @file HttpMethodHandler.hpp
 * @brief Handles HTTP method processing for webserver requests
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-14
 * @version 1.0
 *
 * This file contains the HttpMethodHandler class which is responsible for
 * processing different HTTP methods (GET, POST, DELETE) and generating
 * appropriate responses based on the server configuration.
 *
 * @see GET request method:
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/GET
 * @see POST request method:
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/POST
 * @see DELETE request method:
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/DELETE
 */

#ifndef _HTTP_METHOD_HANDLER_HPP
#define _HTTP_METHOD_HANDLER_HPP

#include <ctime>
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

/**
 * @class HttpMethodHandler
 * @brief Processes HTTP method requests and generates responses
 *
 * Key responsibilities:
 * - Process HTTP GET requests (static files, directory listings)
 * - @todo Handle HTTP POST requests (file uploads)
 * - @todo Manage HTTP DELETE requests (file deletion)
 * - Serve static files with appropriate MIME types
 * - Generate directory listings when auto-index is enabled
 *
 * @see Common helper functions in srcs/HttpUtils.cpp
 */
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
  HttpResponse handlePostMethod(const std::string& path,
                                const HttpRequest& request);
  HttpResponse handleDeleteMethod(const std::string& path);

 protected:
  // helper functions
  HttpResponse serveStaticFile(const std::string& path);
  HttpResponse serveDirectoryContent(const std::string& path,
                                     const std::string& uri);
  HttpResponse handleMultipartFileUpload(const HttpRequest& request,
                                         const std::string& path,
                                         const std::string& content_type);
  bool saveUploadedFile(const std::string& upload_dir,
                        const std::string& file_name,
                        const std::string& content, std::string& error_msg);
  bool isAllowedFileType(const std::string& extention);
  std::string generateFileName(std::string& extension);
};

#endif  /// _HTTP_METHOD_HANDLER_HPP
