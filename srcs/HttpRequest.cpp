/**
 * @file HttpRequest.cpp
 * @brief HTTP Request storage and representation class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-30
 * @version 1.0
 *
 */

#include "HttpRequest.hpp"

// Constructor and destructor

HttpRequest::HttpRequest()
    : _method_code(HttpMethod::UNKNOWN),
      _method_raw(""),
      _request_target(""),
      _http_version(""),
      _headers(),
      _body(""),
      _body_length(0),
      _status() {}

HttpRequest::~HttpRequest() { _headers.clear(); }

// Setters

/**
 * @brief Set HTTP method and convert to enum
 * @param method HTTP method string (case-sensitive)
 */
void HttpRequest::setMethod(const std::string& method) {
  _method_raw = method;

  if (method == "GET") {
    _method_code = HttpMethod::GET;
  } else if (method == "HEAD") {
    _method_code = HttpMethod::HEAD;
  } else if (method == "POST") {
    _method_code = HttpMethod::POST;
  } else if (method == "PUT") {
    _method_code = HttpMethod::PUT;
  } else if (method == "DELETE") {
    _method_code = HttpMethod::DELETE;
  } else if (method == "CONNECT") {
    _method_code = HttpMethod::CONNECT;
  } else if (method == "OPTIONS") {
    _method_code = HttpMethod::OPTIONS;
  } else if (method == "TRACE") {
    _method_code = HttpMethod::TRACE;
  } else {
    _method_code = HttpMethod::UNKNOWN;
  }
}

/**
 * @brief Set request target URI
 * @param request_target The request target string
 */
void HttpRequest::setRequestTarget(const std::string& request_target) {
  _request_target = request_target;
}

/**
 * @brief Set HTTP version
 * @param http_version HTTP version string (e.g., "HTTP/1.1")
 */
void HttpRequest::setHttpVersion(const std::string& http_version) {
  _http_version = http_version;
}

/**
 * @brief Insert header field (case-insensitive)
 *
 * Handles duplicates by adding the value to the existing one, separated by a
 * comma `,`
 *
 * @param field_name Header field name
 * @param value Header field value
 */
void HttpRequest::insertHeader(const std::string& field_name,
                               std::string& value) {
  std::string lowercase_name = toLowerCase(field_name);
  auto it = _headers.find(lowercase_name);
  if (it != _headers.end()) {
    it->second += "," + value;
  } else {
    _headers.insert({lowercase_name, value});
  }
}

/**
 * @brief Set message body
 * @param body HTTP message body content
 */
void HttpRequest::setBody(const std::string& body) { _body = body; }

/**
 * @brief Set success status
 * @param status_code HTTP status code (optional, default = 200)
 */
void HttpRequest::setSuccessStatus(int status_code) {
  _status.result = HttpRequestResult::SUCCESS;
  _status.status_code = status_code;
}

/**
 * @brief Set error status
 * @param error_msg Error message
 * @param error_code HTTP error code
 */
void HttpRequest::setErrorStatus(const std::string& error_msg, int error_code) {
  _status.result = HttpRequestResult::ERROR;
  _status.message = error_msg;
  _status.status_code = error_code;
}

/**
 * @brief Set body length
 * @param content_length Content length in bytes
 */
void HttpRequest::setBodyLength(size_t content_length) {
  _body_length = content_length;
}

// Getters

/**
 * @brief Get raw HTTP method string
 * @return const std::string& Original method string
 */
const std::string& HttpRequest::getMethod(void) const { return _method_raw; }

/**
 * @brief Get HTTP method enum
 * @return const HttpMethod& Parsed method enum
 */
const HttpMethod& HttpRequest::getMethodCode(void) const {
  return _method_code;
}

/**
 * @brief Get request target
 * @return const std::string& Request target URI
 */
const std::string& HttpRequest::getRequestTarget(void) const {
  return _request_target;
}

/**
 * @brief Get HTTP version
 * @return const std::string& HTTP version string
 * @note Expected "HTTP/1.x" (case-sensitive)
 */
const std::string& HttpRequest::getHttpVersion(void) const {
  return _http_version;
}

/**
 * @brief Get header value (case-insensitive lookup)
 * @param field_name Header field name
 * @return const std::string& Header value or empty string if not found
 */
const std::string& HttpRequest::getHeader(const std::string& field_name) const {
  static const std::string empty_string = "";
  std::string lowercase_name = toLowerCase(field_name);
  auto it = _headers.find(lowercase_name);
  if (it != _headers.end()) {
    return it->second;
  }
  return empty_string;
}

/**
 * @brief Check if header exists (case-insensitive)
 * @param field_name Header field name
 * @return true if header exists, false otherwise
 */
bool HttpRequest::hasHeader(const std::string& field_name) const {
  std::string lowercase_name = toLowerCase(field_name);
  return _headers.find(lowercase_name) != _headers.end();
}

/**
 * @brief Get message body
 * @return const std::string& HTTP message body content
 */
const std::string& HttpRequest::getBody(void) const { return _body; }

/**
 * @brief Get body length
 * @return size_t Body length in bytes
 */
size_t HttpRequest::getBodyLength(void) const { return _body_length; }

/**
 * @brief Get request status
 * @return const HttpRequestState& Current request state
 */
const HttpRequestState& HttpRequest::getStatus(void) const { return _status; }

/**
 * @brief Check if request is valid
 * @return true if request parsed successfully, false otherwise
 */
bool HttpRequest::isValid(void) const {
  return _status.result == HttpRequestResult::SUCCESS;
}

// Helpers

/**
 * @brief Constructor for HttpRequestState
 * @param res Result type
 * @param msg Status message
 * @param code Status code
 */
HttpRequestState::HttpRequestState(HttpRequestResult res,
                                   const std::string& msg, int code)
    : result(res), message(msg), status_code(code) {}
