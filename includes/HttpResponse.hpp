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

#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "HttpRequest.hpp"
#include "HttpUtils.hpp"

class HttpRequest;

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();
  HttpResponse& operator=(const HttpResponse& other);
  HttpResponse(const HttpResponse& other);

  void setStatusCode(const HttpUtils::HttpStatusCode& code);
  void setBody(const std::string& body,
               const std::string& content_type = "text/plain");
  void setContentType(const std::string& content_type);
  void setConnectionHeader(const std::string& request_connection,
                           const std::string& request_http_version);
  void insertHeader(const std::string& field_name, const std::string& value);

  void setErrorPageBody(const ConfigParser::ServerConfig& server_config);
  void setErrorResponse(const HttpUtils::HttpStatusCode& code,
                        const std::string& message);

  bool isError(void) const;
  bool isKeepAliveConnection(void) const;

  const std::string& getBody(void) const;
  HttpUtils::HttpStatusCode getStatusCode(void) const;
  std::string getStatusLine(void) const;

  std::string convertToString(void);

 private:
  HttpUtils::HttpStatusCode _status_code;
  std::map<std::string, std::string> _headers;
  std::string _body;
  std::string _content_type;
  bool _is_error_response;
  bool _is_keep_alive_connection;

 private:
  std::string whatReasonPhrase(const HttpUtils::HttpStatusCode& code) const;
  std::string whatDateGMT(void);
  std::string capitalizeHeaderFieldName(const std::string& field_name);
  void setDefaultCatErrorPage(void);
};

#endif  // _HTTP_RESPONSE_HPP
