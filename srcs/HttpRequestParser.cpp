/**
 * @file HttpRequestParser.cpp
 * @brief HTTP Request parsing and handling class implementation
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-25
 *
 * Implementation of the HttpRequestParser class for parsing
 * and validating HTTP/1.1 requests. Processes raw socket data
 * into structured HTTP request objects.
 *
 * @note for now do not trim any extra spaces
 *
 * @see HttpRequestParser.hpp for class interface and TODO items
 * @see https://tools.ietf.org/html/rfc7230 (HTTP/1.1 Message Syntax)
 */

#include "HttpRequestParser.hpp"

#include <sstream>

HttpRequestParser::HttpRequestParser() {}

int HttpRequestParser::parseRequest(const std::string& raw_request,
                                    HttpRequest& request) {
  if (raw_request.empty()) {
    request.setErrorStatus("Empty request",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  std::string_view message = raw_request;

  // step 1: handle request line
  size_t request_line_end = message.find("\r\n");
  if (request_line_end == std::string_view::npos) {
    request.setErrorStatus(
        "HTTP messages MUST use \\r\\n (CRLF) as line terminators",
        HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  std::string_view request_line = message.substr(0, request_line_end);
  if (parseRequestLine(request_line, request)) {
    return PARSE_ERROR;
  }

  // step 2: handle headers
  size_t headers_start = request_line_end + 2;
  if (headers_start >= message.length()) {
    request.setErrorStatus("Missing headers section",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  size_t headers_end = message.find("\r\n\r\n", headers_start);
  bool has_body = (headers_end != std::string_view::npos);

  if (!has_body) {
    if (message.substr(message.length() - 2) != "\r\n") {
      request.setErrorStatus("HTTP messages MUST end with \\r\\n (CRLF)",
                             HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }
    headers_end = message.length() - 2;
  }

  if (headers_end > headers_start) {
    std::string_view headers_content =
        message.substr(headers_start, headers_end - headers_start);
    if (parseRequestHeaders(headers_content, request)) {
      return PARSE_ERROR;
    }
  }

  // step 3: handle body (if present)
  if (has_body) {
    size_t body_start = headers_end + 4;
    if (body_start < message.length()) {
      std::string_view body_content = message.substr(body_start);
      if (parseRequestBody(body_content, request)) {
        return PARSE_ERROR;
      }
    }
  }

  return PARSE_SUCCESS;
}

int HttpRequestParser::parseRequestLine(std::string_view request_line,
                                        HttpRequest& request) {
  if (request_line.empty()) {
    request.setErrorStatus("Empty request line",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  // find method position
  size_t method_end = request_line.find(' ');
  if (method_end == std::string_view::npos) {
    request.setErrorStatus("Malformed request line - missing spaces",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  // find target position
  size_t target_start = method_end + 1;
  size_t target_end = request_line.find(' ', target_start);
  if (target_end == std::string_view::npos) {
    request.setErrorStatus("Malformed request line - missing target",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  // find version position
  size_t version_start = target_end + 1;

  // extract tokens
  std::string method = std::string(request_line.substr(0, method_end));
  std::string target =
      std::string(request_line.substr(target_start, target_end - target_start));
  std::string version = std::string(request_line.substr(version_start));

  // set and validate method
  request.setMethod(method);
  if (request.getMethodCode() == HttpMethod::UNKNOWN) {
    request.setErrorStatus(
        "Method is unrecognized or not implemented: " + method,
        HttpRequestParserError::METHOD_NOT_RECOGNIZED);
    return PARSE_ERROR;
  }

  // set and validate target
  if (!validateRequestTarget(target, request)) {
    return PARSE_ERROR;
  }
  request.setRequestTarget(target);

  // set and validate version
  if (!validateHttpVersion(version, request)) {
    return PARSE_ERROR;
  }
  request.setHttpVersion(version);

  return PARSE_SUCCESS;
}

int HttpRequestParser::parseRequestHeaders(std::string_view headers,
                                           HttpRequest& request) {
  (void)request;
  (void)headers;
  return PARSE_SUCCESS;
}

int HttpRequestParser::parseRequestBody(std::string_view body,
                                        HttpRequest& request) {
  request.setBody(std::string(body));
  return PARSE_SUCCESS;
}

bool HttpRequestParser::validateRequestTarget(const std::string& target,
                                              HttpRequest& request) {
  std::string lower_target = toLowerCase(target);
  if (lower_target.length() > MAX_REQUEST_TARGET_LENGTH) {
    request.setErrorStatus("Request target too long: " + target,
                           HttpRequestParserError::BAD_REQUEST);
    return false;
  }

  for (char ch : lower_target) {
    if (ch < 32 || ch == 127 || ch == '<' || ch == '>' || ch == '"' ||
        ch == '\\') {
      request.setErrorStatus(
          "Request target contains forbidden character: " + ch,
          HttpRequestParserError::BAD_REQUEST);
      return false;
    }
  }

  size_t scheme_end = lower_target.find("://");
  if (scheme_end != std::string::npos) {
    std::string scheme = lower_target.substr(0, scheme_end);
    if (scheme != "http" && scheme != "https") {
      request.setErrorStatus("Only http/https schemes allowed: " + scheme,
                             HttpRequestParserError::BAD_REQUEST);
      return false;
    }
  }

  if (scheme_end == std::string::npos && lower_target[0] != '/') {
    request.setErrorStatus("Request target witout scheme shoul start with '/'",
                           HttpRequestParserError::BAD_REQUEST);
    return false;
  }

  return true;
}

bool HttpRequestParser::validateHttpVersion(const std::string& version,
                                            HttpRequest& request) {
  static const std::regex general_version_regex(R"(^HTTP/\d+\.\d+$)");
  static const std::regex supported_version_regex(R"(^HTTP/1\.[01]$)");

  bool is_valid = std::regex_match(version, general_version_regex);
  bool is_supported = std::regex_match(version, supported_version_regex);

  if (is_valid && !is_supported) {
    request.setErrorStatus(
        "Valid format but unsupported HTTP version: " + version,
        HttpRequestParserError::HTTP_VERSION_NOT_SUPPORTED);
    return false;
  }

  if (!is_valid) {
    request.setErrorStatus("Invalid HTTP version format: " + version,
                           HttpRequestParserError::BAD_REQUEST);
    return false;
  }

  return true;
}
