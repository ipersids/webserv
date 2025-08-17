/**
 * @file HttptUtils.hpp
 * @brief Utility functions for HTTP request processing
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 * @version 1.0
 *
 * This header provides utility functions used by the HttpRequest and
 * HttpRequestParser classes for string manipulation and HTTP-specific
 * operations.
 *
 */

#ifndef _HTTP_UTILS_HPP
#define _HTTP_UTILS_HPP

#include <algorithm>
#include <filesystem>
#include <string>

#include "config.hpp"

#define CRLF_LENGTH 2

namespace HttpUtils {

// Response Status Codes:
// https://datatracker.ietf.org/doc/html/rfc7231#autoid-58
enum class HttpStatusCode {
  UKNOWN = 0,
  // Informational 1xx
  CONTINUE = 100,
  SWITCHING_PROTOCOLS = 101,
  // Successful 2xx
  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NON_AUTHORITATIVE_INFO = 203,
  NO_CONTENT = 204,
  RESET_CONTENT = 205,
  PARTIAL_CONTENT = 206,
  // Redirection 3xx
  MULTIPLE_CHOICES = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  SEE_OTHER = 303,
  NOT_MODIFIED = 304,
  USE_PROXY = 305,
  TEMPORARY_REDIRECT = 307,
  // Client Error 4xx
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  PROXY_AUTHENTICATION_REQUIRED = 407,
  REQUEST_TIMEOUT = 408,
  CONFLICT = 409,
  GONE = 410,
  LENGTH_REQUIRED = 411,
  PAYLOAD_TOO_LARGE = 413,
  URI_TOO_LONG = 414,
  UNSUPPORTED_MEDIA_TYPE = 415,
  RANGE_NOT_SATISFIABLE = 416,
  EXPECTATION_FAILED = 417,
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status/418
  I_AM_TEAPOD = 418,
  TOO_MANY_REQUESTS = 429,
  // Server Error 5xx
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  SERVICE_UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  HTTP_VERSION_NOT_SUPPORTED = 505
};

/**
 * @brief Convert string to lowercase
 *
 * @param str The input string to convert
 * @return std::string A new string with all characters converted to lowercase
 *
 * @warning This function only handles ASCII characters correctly.
 */
std::string toLowerCase(const std::string& str);

const ConfigParser::LocationConfig* getLocation(
    const std::string& request_uri, const ConfigParser::ServerConfig& config);

const std::string getFilePath(const ConfigParser::LocationConfig& location,
                              const std::string& request_uri);

bool isFilePathSecure(const std::string& path, const std::string& root,
                      std::string& message);

bool isMethodAllowed(const ConfigParser::LocationConfig& location,
                     const std::string& method);

}  // namespace HttpUtils

#endif  // _HTTP_UTILS_HPP
