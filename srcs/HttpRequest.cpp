/**
 * @file HttpRequest.cpp
 * @brief HTTP Request storage and representation class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-22
 * @version 2.0
 *
 */

#include "HttpRequest.hpp"

// Constructor and destructor

HttpRequest::HttpRequest()
    : _buffer(""),
      _parsed_buffer_offset(0),
      _method_code(HttpMethod::UNKNOWN),
      _method_raw(""),
      _request_target(""),
      _http_version(""),
      _headers(),
      _body(""),
      _body_length(0),
      _state(HttpParsingState::REQUEST_LINE),
      _is_chanked(false),
      _expected_chunk_length(0),
      _is_error(false),
      _status_code(HttpUtils::HttpStatusCode::I_AM_TEAPOD),
      _err_message("") {}

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
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
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
 * @brief Set error status
 * @param error_msg Error message
 * @param error_code HTTP error code
 */
void HttpRequest::setErrorStatus(const std::string& error_msg,
                                 HttpUtils::HttpStatusCode error_code) {
  _state = HttpParsingState::COMPLETE;
  _is_error = true;
  _err_message = error_msg;
  _status_code = error_code;
}

/**
 * @brief Set body length
 * @param content_length Content length in bytes
 */
void HttpRequest::setBodyLength(size_t content_length) {
  _body_length = content_length;
}

void HttpRequest::appendBuffer(std::string&& data) {
  _buffer += std::move(data);

  if (_parsed_buffer_offset >= PARSED_OFFSET_THRESHOLD) {
    eraseParsedBuffer();
  }
}

void HttpRequest::setChunkedStatus(bool is_chanked) {
  _is_chanked = is_chanked;
}

void HttpRequest::setExpectedChunkLength(size_t expected_length) {
  _expected_chunk_length = expected_length;
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
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
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
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
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

std::string_view HttpRequest::getUnparsedBuffer(void) const {
  if (_parsed_buffer_offset >= _buffer.size()) {
    return std::string_view{};
  }
  return std::string_view{_buffer}.substr(_parsed_buffer_offset);
}

void HttpRequest::eraseParsedBuffer(size_t bytes) {
  size_t amount = (bytes == 0) ? _parsed_buffer_offset
                               : std::min(bytes, _parsed_buffer_offset);

  _buffer.erase(0, amount);
  _parsed_buffer_offset -= amount;
}

void HttpRequest::commitParsedBytes(size_t bytes) {
  _parsed_buffer_offset += bytes;
}

bool HttpRequest::isErrorStatusCode(void) const {
  return _state == HttpParsingState::COMPLETE && _is_error;
}

void HttpRequest::setParsingState(HttpParsingState state) {
  _state = state;
  if (state == HttpParsingState::COMPLETE) {
    _buffer.clear();
    _parsed_buffer_offset = 0;
  } else if (_parsed_buffer_offset >= PARSED_OFFSET_THRESHOLD) {
    eraseParsedBuffer();
  }
}

HttpParsingState HttpRequest::getParsingState(void) const { return _state; }

bool HttpRequest::getChunkedStatus(void) const { return _is_chanked; }

size_t HttpRequest::getExpectedChunkLength(void) const {
  return _expected_chunk_length;
}

void HttpRequest::appendBody(std::string&& data) {
  _body_length += data.length();
  _body += std::move(data);
}

HttpUtils::HttpStatusCode HttpRequest::getStatusCode(void) const {
  return _status_code;
}

const std::string& HttpRequest::getErrorMessage(void) const {
  return _err_message;
}

void HttpRequest::reset(void) {
  _buffer.clear();
  _parsed_buffer_offset = 0;
  _method_code = HttpMethod::UNKNOWN;
  _method_raw.clear();
  _request_target.clear();
  _http_version.clear();
  _headers.clear();
  _body.clear();
  _body_length = 0;
  _state = HttpParsingState::REQUEST_LINE;
  _is_chanked = false;
  _expected_chunk_length = 0;
  _is_error = false;
  _status_code = HttpUtils::HttpStatusCode::I_AM_TEAPOD;
  _err_message.clear();
}

std::string HttpRequest::getRequestLine(void) const {
  std::stringstream request_line;
  request_line << getMethod() << " " << getRequestTarget() << " "
               << getHttpVersion();
  return request_line.str();
}
