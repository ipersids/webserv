/**
 * @file HttpRequest.hpp
 * @brief HTTP Request storage and representation class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 *
 * This class represents an HTTP request as defined in RFC 7230.
 * It provides functionality to store and retrieve HTTP request components
 * including method, target, version, headers, and body.
 *
 * @note This is a storage class - parsing functionality is implemented
 *       in a separate HttpRequestParser class.
 * @note Header field names are case-insensitive per RFC 7230 Section 3.2
 *
 * @todo Handle duplicate header field names according to RFC 7230 Section 3.2.2
 */

#ifndef _HTTP_REQUEST_HPP
#define _HTTP_REQUEST_HPP

#include <algorithm>
#include <iostream>
#include <map>
#include <string>

#include "HttpRequestUtils.hpp"

// Common methods https://datatracker.ietf.org/doc/html/rfc7231#section-4
// Registry: https://www.iana.org/assignments/http-methods/http-methods.xhtml
enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  UNKNOWN
};

enum class HttpRequestResult { PROCESSING, SUCCESS, ERROR };

struct HttpRequestState {
  HttpRequestResult result;
  std::string message;
  int status_code;

  HttpRequestState(HttpRequestResult res = HttpRequestResult::PROCESSING,
                   const std::string& msg = "", int code = -1);
};

class HttpRequest {
 public:
  HttpRequest();
  ~HttpRequest();
  HttpRequest& operator=(const HttpRequest& other) = delete;
  HttpRequest(const HttpRequest& other) = delete;

  void setMethod(const std::string& method);
  void setRequestTarget(const std::string& request_target);
  void setHttpVersion(const std::string& http_version);
  void insertHeader(const std::string& field_name, std::string& value);
  void setBody(const std::string& body);
  void setSuccessStatus(int status_code = 200);
  void setErrorStatus(const std::string& error_msg, int error_code);

  const std::string& getMethod(void) const;
  const HttpMethod& getMethodCode(void) const;
  const std::string& getRequestTarget(void) const;
  const std::string& getHttpVersion(void) const;
  const std::string& getHeader(const std::string& field_name) const;
  bool hasHeader(const std::string& field_name) const;
  const std::string& getBody(void) const;
  const HttpRequestState& getStatus(void) const;

  bool isValid(void) const;

 private:
  // Request Line https://datatracker.ietf.org/doc/html/rfc7230#autoid-17
  HttpMethod _method_code;
  std::string _method_raw;
  std::string _request_target;
  std::string _http_version;
  // Header Fields https://datatracker.ietf.org/doc/html/rfc7230#autoid-19
  std::map<std::string, std::string> _headers;
  // Message Body https://datatracker.ietf.org/doc/html/rfc7230#autoid-26
  std::string _body;
  // Error managment
  HttpRequestState _status;
};

#endif
