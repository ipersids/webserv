/**
 * @file HttpRequest.cpp
 * @brief HTTP Request parsing and handling class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 *
 */

#include "HttpRequest.hpp"

/** Constructor and distructor */

HttpRequest::HttpRequest()
 : _method_code(HttpMethod::UNKNOWN), _method_raw(""), _request_target(""),
   _http_version(""), _headers(), _body("") {
}

HttpRequest::~HttpRequest() {
  _headers.clear();
}

/** Setters */

void HttpRequest::setMethod(const std::string& method) {
  _method_raw = method;

  if (method == "GET") {
    _method_code = HttpMethod::GET;
  } else if (method == "POST") {
    _method_code = HttpMethod::POST;
  } else if (method == "DELETE") {
    _method_code = HttpMethod::DELETE;
  } else {
    _method_code = HttpMethod::UNKNOWN;
  }
}

void HttpRequest::setRequestTarget(const std::string& request_target) {
  _request_target = request_target;
}

void HttpRequest::setHttpVersion(const std::string& http_version) {
  _http_version = http_version;
}

void HttpRequest::insertHeader(const std::string& field_name,
                               std::string& value) {
  std::string lowercase_name = toLowerCase(field_name);
  _headers.insert({lowercase_name, value});
}

void HttpRequest::setBody(const std::string& body) {
  _body = body;
}

/** Getters */

const std::string& HttpRequest::getMethod(void) const {
  return _method_raw;
}

const HttpMethod& HttpRequest::getMethodCode(void) const {
  return _method_code;
}

const std::string& HttpRequest::getRequestTarget(void) const {
  return _request_target;
}

const std::string& HttpRequest::getHttpVersion(void) const {
  return _http_version;
}

const std::string& HttpRequest::getHeader(const std::string& field_name) const {
  static const std::string empty_string = "";
  std::string lowercase_name = toLowerCase(field_name);
  auto it = _headers.find(lowercase_name);
  if (it != _headers.end()) {
    return it->second;
  }
  return empty_string;
}

bool HttpRequest::hasHeader(const std::string& field_name) const {
  std::string lowercase_name = toLowerCase(field_name);
  return _headers.find(lowercase_name) != _headers.end();
}

const std::string& HttpRequest::getBody(void) const {
  return _body;
}

/** Helpers */

std::string HttpRequest::toLowerCase(const std::string& str) const {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}
