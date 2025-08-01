/**
 * @file HttpResponse.hpp
 * @brief HTTP Response storage and representation class
 * @author Your Name
 * @date 2025-07-31
 * @version 1.0
 *
 * @todo
 * - for now we have list of status codes mentioned in RFC 7231, but later
 *   we can keep only statuses, what we support and averwase give "I'm teapod"
 *
 * @todo convertToString() -> check mandatory headers (host, content size...)
 *
 * @see https://datatracker.ietf.org/doc/html/rfc7231#autoid-58 Status Codes
 * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status
 * Mozilla HTTP response status codes
 * @see https://datatracker.ietf.org/doc/html/rfc7231#autoid-101 Header Fields
 */

#ifndef _HTTP_RESPONSE_HPP
#define _HTTP_RESPONSE_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "HttpRequest.hpp"
#include "HttpUtils.hpp"

class HttpRequest;

class HttpResponse {
 public:
  HttpResponse(
      const HttpUtils::HttpStatusCode& code = HttpUtils::HttpStatusCode::OK,
      const std::string& message = "");
  ~HttpResponse();
  HttpResponse& operator=(const HttpResponse& other) = delete;
  HttpResponse(const HttpResponse& other) = delete;

  void setHttpVersion(const std::string& version);
  void setStatusCode(const HttpUtils::HttpStatusCode& code);
  void insertHeader(const std::string& field_name, const std::string& value);
  void setBody(const std::string& body);

  const std::string& getHttpVersion(void) const;
  const HttpUtils::HttpStatusCode& getStatusCode(void) const;
  const std::string& getReasonPhrase(void) const;
  const std::string& getHeader(const std::string& field_name) const;
  const std::string& getBody(void) const;

  std::string convertToString(void);

  bool hasHeader(const std::string& field_name) const;

 private:
  // Status Line: https://datatracker.ietf.org/doc/html/rfc7230#autoid-18
  std::string _http_version;
  HttpUtils::HttpStatusCode _status_code;
  std::string _reason_phrase;
  // Header Fields: https://datatracker.ietf.org/doc/html/rfc7231#autoid-101
  std::map<std::string, std::string> _headers;
  // Response Body
  std::string _body;

 private:
  std::string whatReasonPhrase(const HttpUtils::HttpStatusCode& code);
  std::string capitalizeHeaderFieldName(const std::string& field_name);
  void removeHeader(const std::string& field_name);
};

#endif  // _HTTP_RESPONSE_HPP
