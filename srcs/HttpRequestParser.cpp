/**
 * @file HttpRequestParser.cpp
 * @brief HTTP Request parsing and handling class implementation
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-22
 * @version 2.0
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

// Parsing

/**
 * @brief Parse raw HTTP request string into HttpRequest object
 * @param data Raw HTTP request data
 * @param request HttpRequest object to populate
 * @return HttpRequestParser::Status::DONE of
 * HttpRequestParser::Status::INPROGRESS
 */
HttpRequestParser::Status HttpRequestParser::parseRequest(
    std::string&& data, HttpRequest& request) {
  if (data.empty()) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }

  request.appendBuffer(std::move(data));

  HttpRequestParser::Status status;
  while (true) {
    switch (request.getParsingState()) {
      // step 1: handle request line
      case HttpParsingState::REQUEST_LINE:
        status = parseRequestLine(request);
        if (status != HttpRequestParser::Status::CONTINUE) {
          return status;
        }
        break;
      // step 2: handle headers
      case HttpParsingState::HEADERS:
        status = parseRequestHeaders(request);
        if (status != HttpRequestParser::Status::CONTINUE) {
          return status;
        }
        break;
      // step 3.0: handle simple body (if present)
      case HttpParsingState::BODY:
        status = parseRequestBody(request);
        if (status != HttpRequestParser::Status::CONTINUE) {
          return status;
        }
        break;

      // step 3.1: handle chunked body size
      case HttpParsingState::CHUNKED_BODY_SIZE:
        status = parseRequestChunkedBodySize(request);
        if (status != HttpRequestParser::Status::CONTINUE) {
          return status;
        }
        break;

      // step 3.2: handle chunked body data
      case HttpParsingState::CHUNKED_BODY_DATA:
        status = parseRequestChunkedBodyData(request);
        if (status != HttpRequestParser::Status::CONTINUE) {
          return status;
        }
        break;

      // step 3.3: handle end of chunked request
      case HttpParsingState::CHUNKED_BODY_TRAILER:
        status = parseRequestChunkedBodyTrailer(request);
        if (status != HttpRequestParser::Status::CONTINUE) {
          return status;
        }
        break;

      case HttpParsingState::COMPLETE:
        return HttpRequestParser::Status::DONE;

      default:
        // unknown state (should never happens)
        return HttpRequestParser::Status::ERROR;
    }
  }

  return HttpRequestParser::Status::DONE;
}

/**
 * @brief Parse HTTP request line (method, target, version)
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
HttpRequestParser::Status HttpRequestParser::parseRequestLine(
    HttpRequest& request) {
  std::string_view message = request.getUnparsedBuffer();

  size_t request_line_end = std::string_view::npos;
  request_line_end = message.find("\r\n");
  if (request_line_end == std::string_view::npos) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }
  // get request line
  std::string_view request_line = message.substr(0, request_line_end);
  if (request_line.empty()) {
    request.setErrorStatus("Empty request line",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }
  // find method position
  size_t method_end = request_line.find(' ');
  if (method_end == std::string_view::npos) {
    request.setErrorStatus("Malformed request line - missing spaces",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }
  // find target position
  size_t target_start = method_end + 1;
  size_t target_end = request_line.find(' ', target_start);
  if (target_end == std::string_view::npos) {
    request.setErrorStatus("Malformed request line - missing target",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
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
        HttpUtils::HttpStatusCode::NOT_IMPLEMENTED);
    return HttpRequestParser::Status::ERROR;
  }
  // set and validate target
  if (!validateRequestTarget(target, request)) {
    return HttpRequestParser::Status::ERROR;
  }
  request.setRequestTarget(target);
  // set and validate version
  if (!validateHttpVersion(version, request)) {
    return HttpRequestParser::Status::ERROR;
  }
  request.setHttpVersion(version);
  request.commitParsedBytes(request_line.length() + 2);
  request.setParsingState(HttpParsingState::HEADERS);
  return HttpRequestParser::Status::CONTINUE;
}

/**
 * @brief Parse HTTP headers section
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
HttpRequestParser::Status HttpRequestParser::parseRequestHeaders(
    HttpRequest& request) {
  std::string_view message = request.getUnparsedBuffer();

  if (message.find("\r\n") == 0) {
    request.setErrorStatus("Malformed header - missing Host",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }

  size_t headers_end = message.find("\r\n\r\n");
  if (headers_end == std::string_view::npos) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }

  std::string_view headers_content = message.substr(0, headers_end);

  size_t header_counter = 0;
  size_t start_pos = 0;

  while (start_pos < headers_content.size()) {
    size_t end_pos = headers_content.find("\r\n", start_pos);
    // no more CRLF found
    if (end_pos == std::string_view::npos) {
      end_pos = headers_content.size();
    }

    std::string_view header_line =
        headers_content.substr(start_pos, end_pos - start_pos);

    if (parseRequestHeaderLine(header_line, request) ==
        HttpRequestParser::Status::ERROR) {
      return HttpRequestParser::Status::ERROR;
    }

    header_counter += 1;

    if (end_pos == headers_content.size()) {
      break;
    }
    start_pos = end_pos + 2;
  }

  // parsed bytes: headers + "\r\n\r\n"
  request.commitParsedBytes(headers_end + 4);

  if (!validateHeadersSetup(request)) {
    return HttpRequestParser::Status::ERROR;
  }

  return HttpRequestParser::Status::CONTINUE;
}

/**
 * @brief Parse single HTTP header line
 * @param header Header line string view
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
HttpRequestParser::Status HttpRequestParser::parseRequestHeaderLine(
    std::string_view header, HttpRequest& request) {
  size_t colon_pos = header.find(':');
  if (colon_pos == std::string_view::npos) {
    request.setErrorStatus(
        "Malformed header - missing colon: " + std::string(header),
        HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }

  std::string field_name = std::string(header.substr(0, colon_pos));
  std::string value = std::string(header.substr(colon_pos + 1));

  if (!validateHeaderField(field_name)) {
    request.setErrorStatus(
        "Malformed header - invalid field name in line: " + std::string(header),
        HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }
  if (!validateAndTrimHeaderValue(value)) {
    request.setErrorStatus(
        "Malformed header - invalid value in line: " + std::string(header),
        HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }

  request.insertHeader(field_name, value);

  return HttpRequestParser::Status::CONTINUE;
}

/**
 * @brief Parse and validate HTTP message body
 * @param body Body string view
 * @param request HttpRequest object to populate
 * @return PARSE_SUCCESS on success, PARSE_ERROR on failure
 */
HttpRequestParser::Status HttpRequestParser::parseRequestBody(
    HttpRequest& request) {
  std::string_view message = request.getUnparsedBuffer();
  if (request.getBodyLength() > message.length()) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }

  if (request.getBodyLength() != message.length()) {
    request.setErrorStatus("Content-Length mismatch: expected " +
                               std::to_string(request.getBodyLength()) +
                               " bytes, got " +
                               std::to_string(message.length()) + " bytes",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }

  request.setBody(std::string(message));
  request.commitParsedBytes(message.length());
  request.setParsingState(HttpParsingState::COMPLETE);

  return HttpRequestParser::Status::CONTINUE;
}

HttpRequestParser::Status HttpRequestParser::parseRequestChunkedBodySize(
    HttpRequest& request) {
  std::string_view message = request.getUnparsedBuffer();

  size_t chunk_size_end = message.find("\r\n");
  if (chunk_size_end == std::string_view::npos) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }

  std::string hex_chunk_size = std::string(message.substr(0, chunk_size_end));
  try {
    size_t chunk_size = std::stoull(hex_chunk_size, 0, 16);
    request.setExpectedChunkLength(chunk_size);
  } catch (const std::exception& e) {
    request.setErrorStatus(
        "Failed to parse chunk data size: " + std::string(e.what()) + " (" +
            hex_chunk_size + ")",
        HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  } catch (...) {
    request.setErrorStatus(
        "Unknown problem during parsing chunk data size: " + hex_chunk_size,
        HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR);
    return HttpRequestParser::Status::ERROR;
  }

  if (request.getExpectedChunkLength() > 0) {
    request.setParsingState(HttpParsingState::CHUNKED_BODY_DATA);
  } else {
    request.setParsingState(HttpParsingState::CHUNKED_BODY_TRAILER);
  }
  request.commitParsedBytes(chunk_size_end + 2);

  return HttpRequestParser::Status::CONTINUE;
}

HttpRequestParser::Status HttpRequestParser::parseRequestChunkedBodyData(
    HttpRequest& request) {
  std::string_view message = request.getUnparsedBuffer();

  size_t chunk_data_end = message.find("\r\n");
  if (chunk_data_end == std::string_view::npos) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }

  std::string chunk_data = std::string(message.substr(0, chunk_data_end));
  if (chunk_data.length() != request.getExpectedChunkLength()) {
    request.setErrorStatus(
        "Chunk length mismatch: expected " +
            std::to_string(request.getExpectedChunkLength()) + " bytes, got " +
            std::to_string(chunk_data.length()) + " bytes",
        HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }

  request.commitParsedBytes(chunk_data_end + 2);
  request.appendBody(std::move(chunk_data));
  request.setParsingState(HttpParsingState::CHUNKED_BODY_SIZE);

  return HttpRequestParser::Status::CONTINUE;
}

HttpRequestParser::Status HttpRequestParser::parseRequestChunkedBodyTrailer(
    HttpRequest& request) {
  std::string_view message = request.getUnparsedBuffer();

  size_t chunk_trailer_end = message.find("\r\n");
  if (chunk_trailer_end == std::string_view::npos) {
    return HttpRequestParser::Status::WAIT_FOR_DATA;
  }

  // the trailer after the last chunk must be just "\r\n"
  if (chunk_trailer_end != 0 || message.length() != 2) {
    request.setErrorStatus("Malformed chunked body trailer",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return HttpRequestParser::Status::ERROR;
  }

  request.commitParsedBytes(chunk_trailer_end + 2);
  request.eraseParsedBuffer();
  request.setParsingState(HttpParsingState::COMPLETE);
  return HttpRequestParser::Status::CONTINUE;
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
  std::string lower_target = HttpUtils::toLowerCase(target);
  if (lower_target.length() > MAX_REQUEST_TARGET_LENGTH) {
    request.setErrorStatus("Request target too long: " + target,
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return false;
  }

  for (char ch : lower_target) {
    if (ch < 32 || ch == 127 || ch == '<' || ch == '>' || ch == '"' ||
        ch == '\\') {
      request.setErrorStatus(
          "Request target contains forbidden character: " + std::string(1, ch),
          HttpUtils::HttpStatusCode::BAD_REQUEST);
      return false;
    }
  }

  size_t scheme_end = lower_target.find("://");
  if (scheme_end != std::string::npos) {
    std::string scheme = lower_target.substr(0, scheme_end);
    if (scheme != "http" && scheme != "https") {
      request.setErrorStatus("Only http/https schemes allowed: " + scheme,
                             HttpUtils::HttpStatusCode::BAD_REQUEST);
      return false;
    }
  }

  if (scheme_end == std::string::npos && lower_target[0] != '/') {
    request.setErrorStatus(
        "Request target without scheme should start with '/'",
        HttpUtils::HttpStatusCode::BAD_REQUEST);
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
        HttpUtils::HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED);
    return false;
  }

  if (!is_valid) {
    request.setErrorStatus("Invalid HTTP version format: " + version,
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
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

bool HttpRequestParser::validateHeadersSetup(HttpRequest& request) {
  // https://datatracker.ietf.org/doc/html/rfc7230#section-5.4
  if (!request.hasHeader("Host")) {
    request.setErrorStatus("Host header is missing",
                           HttpUtils::HttpStatusCode::BAD_REQUEST);
    return false;
  }

  if (request.hasHeader("Transfer-Encoding") &&
      request.hasHeader("Content-Length")) {
    request.setErrorStatus(
        "Malformed header - can't have both Content-Length and "
        "Transfer-Encoding",
        HttpUtils::HttpStatusCode::BAD_REQUEST);
    return false;
  }

  if (request.hasHeader("Transfer-Encoding")) {
    std::string transfer_encoding = request.getHeader("Transfer-Encoding");
    transfer_encoding = HttpUtils::toLowerCase(transfer_encoding);
    if (transfer_encoding.find("chunked") != std::string::npos) {
      request.setChunkedStatus(true);
      request.setParsingState(HttpParsingState::CHUNKED_BODY_SIZE);
    } else {
      request.setErrorStatus(
          "Unsupported Transfer-Encoding type: " + transfer_encoding,
          HttpUtils::HttpStatusCode::BAD_REQUEST);
      return false;
    }
  } else {
    request.setParsingState(HttpParsingState::BODY);
  }

  if (request.hasHeader("Content-Length")) {
    try {
      size_t content_length = std::stoull(request.getHeader("content-length"));
      request.setBodyLength(content_length);
    } catch (const std::exception& e) {
      request.setErrorStatus(
          "Failed to parse Content-Length: " + std::string(e.what()) + " (" +
              request.getHeader("content-length") + ")",
          HttpUtils::HttpStatusCode::BAD_REQUEST);
      return false;
    } catch (...) {
      request.setErrorStatus(
          "Unknown problem during parsing Content-Length occurs: " +
              request.getHeader("content-length"),
          HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR);
      return false;
    }
  }

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
