/**
 * @file HttpRequest.hpp
 * @brief HTTP Request storage and representation class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-22
 * @version 2.0
 *
 * This class represents an HTTP request as defined in RFC 7230.
 * It provides functionality to store and retrieve HTTP request components
 * including method, target, version, headers, and body.
 *
 * @note This is a storage class - parsing functionality is implemented
 *       in a separate HttpRequestParser class.
 * @note Header field names are case-insensitive per RFC 7230 Section 3.2
 */

#ifndef _HTTP_REQUEST_HPP
#define _HTTP_REQUEST_HPP

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "HttpUtils.hpp"

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

enum class HttpParsingState {
  REQUEST_LINE,
  HEADERS,
  BODY,
  CHUNKED_BODY_SIZE,
  CHUNKED_BODY_DATA,
  CHUNKED_BODY_TRAILER,
  COMPLETE
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
  void setBodyLength(size_t content_length);
  void setErrorStatus(const std::string& error_msg,
                      HttpUtils::HttpStatusCode error_code);
  void setParsingState(HttpParsingState state);
  void setChunkedStatus(bool is_chanked);
  void setExpectedChunkLength(size_t expected_length);
  void appendBuffer(std::string&& data);
  void commitParsedBytes(size_t bytes);
  void appendBody(std::string&& data);

  const std::string& getMethod(void) const;
  const HttpMethod& getMethodCode(void) const;
  const std::string& getRequestTarget(void) const;
  const std::string& getHttpVersion(void) const;
  const std::string& getHeader(const std::string& field_name) const;
  const std::string& getBody(void) const;
  size_t getBodyLength(void) const;
  std::string_view getUnparsedBuffer(void) const;
  HttpParsingState getParsingState(void) const;
  bool getChunkedStatus(void) const;
  size_t getExpectedChunkLength(void) const;
  HttpUtils::HttpStatusCode getStatusCode(void) const;
  const std::string& getErrorMessage(void) const;
  std::string getRequestLine(void) const;

  bool hasHeader(const std::string& field_name) const;
  bool isErrorStatusCode(void) const;

  void eraseParsedBuffer(size_t bytes = 0);
  void reset(void);

 private:
  std::string _buffer;
  size_t _parsed_buffer_offset;
  // Request Line https://datatracker.ietf.org/doc/html/rfc7230#autoid-17
  HttpMethod _method_code;
  std::string _method_raw;
  std::string _request_target;
  std::string _http_version;
  // Header Fields https://datatracker.ietf.org/doc/html/rfc7230#autoid-19
  std::map<std::string, std::string> _headers;
  // Message Body https://datatracker.ietf.org/doc/html/rfc7230#autoid-26
  std::string _body;
  size_t _body_length;
  // Parsing managment
  HttpParsingState _state;
  bool _is_chanked;
  size_t _expected_chunk_length;
  bool _is_error;
  HttpUtils::HttpStatusCode _status_code;
  std::string _err_message;

 protected:
  static constexpr size_t PARSED_OFFSET_THRESHOLD = 4096;
};

#endif  // _HTTP_REQUEST_HPP
