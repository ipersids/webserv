/**
 * @file HttpRequestParser.cpp
 * @brief HTTP Request parsing and handling class implementation
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-30
 * @version 1.0
 *
 * Implementation of the HttpRequestParser class for parsing
 * and validating HTTP/1.1 requests. Processes raw socket data
 * into structured HTTP request objects.
 *
 * @note
 * - for now do not trim any extra spaces
 * - some headers can't have multiple values and be combined with comma
 * - some headers might need extra validation ()
 * - we can also add more lenght limits
 *
 * @see HttpRequestParser.hpp for class interface and TODO items
 * @see https://tools.ietf.org/html/rfc7230 (HTTP/1.1 Message Syntax)
 */

#include "HttpRequestParser.hpp"

// Constructors

HttpRequestParser::HttpRequestParser() {}

// Parsing

/**
 * @brief Parse raw HTTP request string into HttpRequest object
 * @param raw_request Raw HTTP request data
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS (0) on success, PARSE_ERROR (128) on failure
 */
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
    request.setErrorStatus("Headers section should have at least Host field",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  size_t headers_end = message.find("\r\n\r\n", headers_start);
  bool has_body = (headers_end != std::string_view::npos);

  if (!has_body) {
    if (message.substr(message.length() - 2) != "\r\n") {
      request.setErrorStatus(
          "HTTP messages MUST use \\r\\n (CRLF) as line terminators",
          HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }
    headers_end = message.length() - 2;
  }

  if (headers_end > headers_start) {
    std::string_view headers_content =
        message.substr(headers_start, headers_end - headers_start + 2);
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

/**
 * @brief Parse HTTP request line (method, target, version)
 * @param request_line Request line string view
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
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

/**
 * @brief Parse HTTP headers section
 * @param headers Headers string view
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
int HttpRequestParser::parseRequestHeaders(std::string_view headers,
                                           HttpRequest& request) {
  size_t header_counter = 0;
  size_t start_pos = 0;

  while (start_pos < headers.size()) {
    if (header_counter >= MAX_REQUEST_HEADERS_COUNT) {
      request.setErrorStatus(
          "Amount of headers exceeded limit: " + std::to_string(header_counter),
          HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }

    size_t end_pos = headers.find("\r\n", start_pos);
    std::string_view header_line =
        headers.substr(start_pos, end_pos - start_pos);
    start_pos = end_pos + 2;

    if (header_line.empty()) {
      request.setErrorStatus(
          "Empty lines in header section are not allowed: line " +
              std::to_string(header_counter + 1),
          HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }

    if (parseRequestHeaderLine(header_line, request) != PARSE_SUCCESS) {
      return PARSE_ERROR;
    }

    header_counter += 1;
  }

  // https://datatracker.ietf.org/doc/html/rfc7230#section-5.4
  if (!request.hasHeader("host")) {
    request.setErrorStatus("Host header is missing",
                           HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  return PARSE_SUCCESS;
}

/**
 * @brief Parse single HTTP header line
 * @param header Header line string view
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
int HttpRequestParser::parseRequestHeaderLine(std::string_view header,
                                              HttpRequest& request) {
  size_t colon_pos = header.find(':');
  if (colon_pos == std::string_view::npos) {
    request.setErrorStatus(
        "Malformed header - missing colon: " + std::string(header),
        HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  std::string field_name = std::string(header.substr(0, colon_pos));
  std::string value = std::string(header.substr(colon_pos + 1));

  if (!validateHeaderField(field_name)) {
    request.setErrorStatus(
        "Malformed header - invalid field name in line: " + std::string(header),
        HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }
  if (!validateAndTrimHeaderValue(value)) {
    request.setErrorStatus(
        "Malformed header - invalid value in line: " + std::string(header),
        HttpRequestParserError::BAD_REQUEST);
    return PARSE_ERROR;
  }

  request.insertHeader(field_name, value);

  return PARSE_SUCCESS;
}

/**
 * @brief Parse and validate HTTP message body
 * @param body Body string view
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
int HttpRequestParser::parseRequestBody(std::string_view body,
                                        HttpRequest& request) {
  if (request.hasHeader("content-length")) {
    if (request.hasHeader("transfer-encoding")) {
      request.setErrorStatus(
          "Malformed header - can't have both Content-Length and "
          "Transfer-Encoding",
          HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }

    try {
      size_t content_length = std::stoull(request.getHeader("content-length"));
      request.setBodyLength(content_length);
    } catch (const std::exception&) {
      request.setErrorStatus("Invalid Content-Length header value: " +
                                 request.getHeader("content-length"),
                             HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }

    if (request.getBodyLength() >= MAX_REQUEST_BODY_SIZE) {
      request.setErrorStatus("Body size " + std::to_string(body.length()) +
                                 " exceeds maximum allowed size " +
                                 std::to_string(MAX_REQUEST_BODY_SIZE),
                             HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }

    // body.length() + 2 -> 2 bytes CRLF included in body lenght
    if (request.getBodyLength() != body.length() + 2) {
      request.setErrorStatus("Content-Length mismatch: expected " +
                                 std::to_string(request.getBodyLength()) +
                                 " bytes, got " +
                                 std::to_string(body.length()) + " bytes",
                             HttpRequestParserError::BAD_REQUEST);
      return PARSE_ERROR;
    }
  } else {
    if (!body.empty() && request.getMethodCode() == HttpMethod::POST) {
      request.setErrorStatus("Content-Length header required",
                             HttpRequestParserError::BODY_LENGTH_REQUIRED);
      return PARSE_ERROR;
    }
  }
  request.setBody(std::string(body));
  return PARSE_SUCCESS;
}

// Validation

/**
 * @brief Validate request target URI format
 * @param target Request target string
 * @param request HttpRequest object for error reporting
 * @return true if valid, false otherwise
 */
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
    request.setErrorStatus(
        "Request target without scheme should start with '/'",
        HttpRequestParserError::BAD_REQUEST);
    return false;
  }

  return true;
}

/**
 * @brief Validate HTTP version format
 * @param version HTTP version string
 * @param request HttpRequest object for error reporting
 * @return true if valid as common type, false otherwise
 */
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

/**
 * @brief Validate header field name format
 * @param field Header field name
 * @return true if valid token characters, false otherwise
 */
bool HttpRequestParser::validateHeaderField(const std::string& field) {
  if (field.empty()) {
    return false;
  }
  for (auto ch : field) {
    if (!validateTokenChar(ch)) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Validate and trim header value
 * @param value Header value to validate and trim (modified in place)
 * @return true if valid after trimming, false otherwise
 */
bool HttpRequestParser::validateAndTrimHeaderValue(std::string& value) {
  size_t start = 0;
  size_t end = value.size();

  while (start < end && (value[start] == ' ' || value[start] == '\t')) {
    ++start;
  }
  while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t')) {
    --end;
  }

  if (start >= end) {
    return false;
  }

  for (size_t i = start; i < end; ++i) {
    unsigned char ch = static_cast<unsigned char>(value[i]);
    if (ch <= 31 || ch == 127 || ch == '\r' || ch == '\n') {
      return false;
    }
  }

  value = value.substr(start, end - start);
  return true;
}

/**
 * @brief Validate single character as HTTP token character
 * @param ch Character to validate
 * @return true if valid token character per RFC 7230, false otherwise
 */
bool HttpRequestParser::validateTokenChar(char ch) {
  // RFC 7230 tchar definition
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
         (ch >= '0' && ch <= '9') || ch == '!' || ch == '#' || ch == '$' ||
         ch == '%' || ch == '&' || ch == '\'' || ch == '*' || ch == '+' ||
         ch == '-' || ch == '.' || ch == '^' || ch == '_' || ch == '`' ||
         ch == '|' || ch == '~';
}
