/**
 * @file HttpRequestParser.hpp
 * @brief HTTP Request parsing and handling class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-22
 * @version 2.0
 *
 * Handles parsing and validation of incoming HTTP/1.1 requests.
 * Extracts method, URI, headers, and body from raw request data.
 *
 * Core Features:
 * - Parse HTTP method, URI, and version
 * - Extract and store headers
 * - Handle request body
 * - Validate request format and completeness
 *
 * @todo Phase 1 (Essential - Core HTTP Parsing):
 * ✓ Parse request line (method, URI, HTTP version)
 * ✓ Header extraction and storage
 * ✓ Basic request validation (malformed syntax)
 *
 * @todo Phase 2 (Body Handling):
 * ✓ Content-Length validation
 * - Body reading
 * - POST data processing
 * - Content-Type header processing
 * - Request completeness checking
 *
 * @todo Phase 3 (Advanced Features):
 * - Chunked transfer encoding support
 * - Keep-alive connection handling
 *
 * @see https://tools.ietf.org/html/rfc7230 (HTTP/1.1 Message Syntax)
 * @see https://tools.ietf.org/html/rfc7231 (HTTP/1.1 Semantics)
 *
 * @note
 * - Request-Line = Method SP Request-URI SP HTTP-Version CRLF
 * - The method is case-sensitive
 * - Servers should return the status code 501 (not implemented)
 *   if the method is unrecognized or not implemented.
 * - The Request-URI is a Uniform Resource Identifier
 * - RFC 7230 Section 3: HTTP messages MUST use \r\n (CRLF) as line
 *   terminators
 */

#ifndef _HTTP_REQUEST_PARSER_HPP
#define _HTTP_REQUEST_PARSER_HPP

#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "HttpRequest.hpp"
#include "HttpUtils.hpp"

#define PARSE_SUCCESS 0
#define PARSE_ERROR 128
#define MAX_REQUEST_TARGET_LENGTH 2048

namespace HttpRequestParser {

enum class Status { WAIT_FOR_DATA, CONTINUE, DONE, ERROR };

Status parseRequest(std::string&& data, HttpRequest& request);

Status parseRequestLine(HttpRequest& request);
Status parseRequestHeaders(HttpRequest& request);
Status parseRequestHeaderLine(std::string_view header, HttpRequest& request);
Status parseRequestBody(HttpRequest& request);
Status parseRequestChunkedBodySize(HttpRequest& request);
Status parseRequestChunkedBodyData(HttpRequest& request);
Status parseRequestChunkedBodyTrailer(HttpRequest& request);

bool validateRequestTarget(const std::string& terget, HttpRequest& request);
bool validateHttpVersion(const std::string& version, HttpRequest& request);
bool validateHeaderField(const std::string& field);
bool validateAndTrimHeaderValue(std::string& value);
bool validateTokenChar(char ch);
bool validateHeadersSetup(HttpRequest& request);

}  // namespace HttpRequestParser

#endif  // _HTTP_REQUEST_PARSER_HPP
