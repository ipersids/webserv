/**
 * @file test_http_request_parser.cpp
 * @brief Unit tests for HttpRequestParser class
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-29
 */

#include <cassert>
#include <iostream>
#include <string>

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"

#define ASSERT_PARSE_SUCCESS(parser, request_str, request_obj) \
  assert(parser.parseRequest(request_str, request_obj) == 0)

#define ASSERT_PARSE_ERROR(parser, request_str, request_obj) \
  assert(parser.parseRequest(request_str, request_obj) != 0)

static void test_invalid_get_request() {
  std::cout << "Testing invalid GET request parsing..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string get =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: 24\r\n"
      "\r\n"
      "{\"name\":\"John\",\"age\":30}";

  ASSERT_PARSE_ERROR(parser, get, request);

  assert(request.getMethod() == "GET");
  assert(request.getMethodCode() == HttpMethod::GET);
  assert(request.getRequestTarget() == "/index.html");
  assert(request.getHttpVersion() == "HTTP/1.1");
  assert(request.hasHeader("host"));
  assert(request.getHeader("host") == "example.com");
  assert(request.hasHeader("content-length"));
  assert(request.getHeader("content-length") == "24");
  assert(request.getBody().empty());

  std::cout << "\t✓ passed" << std::endl;
}

static void test_basic_get_request() {
  std::cout << "Testing basic GET request parsing..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string basic_get =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  ASSERT_PARSE_SUCCESS(parser, basic_get, request);

  assert(request.getMethod() == "GET");
  assert(request.getMethodCode() == HttpMethod::GET);
  assert(request.getRequestTarget() == "/index.html");
  assert(request.getHttpVersion() == "HTTP/1.1");
  assert(request.hasHeader("host"));
  assert(request.getHeader("host") == "example.com");
  assert(request.getBody().empty());

  std::cout << "\t✓ passed" << std::endl;
}

static void test_post_request_with_body() {
  std::cout << "Testing POST request with body..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string post_request =
      "POST /api/users HTTP/1.1\r\n"
      "Host: api.example.com\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 24\r\n"
      "\r\n"
      "{\"name\":\"John\",\"age\":30}";

  ASSERT_PARSE_SUCCESS(parser, post_request, request);

  assert(request.getMethod() == "POST");
  assert(request.getMethodCode() == HttpMethod::POST);
  assert(request.getRequestTarget() == "/api/users");
  assert(request.getHttpVersion() == "HTTP/1.1");
  assert(request.hasHeader("host"));
  assert(request.getHeader("host") == "api.example.com");
  assert(request.hasHeader("content-type"));
  assert(request.getHeader("content-type") == "application/json");
  assert(request.hasHeader("content-length"));
  assert(request.getHeader("content-length") == "24");
  assert(request.getBody() == "{\"name\":\"John\",\"age\":30}");

  std::cout << "\t✓ passed" << std::endl;
}

static void test_delete_request() {
  std::cout << "Testing DELETE request..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string delete_request =
      "DELETE /api/users/123 HTTP/1.1\r\n"
      "Host: api.example.com\r\n"
      "Authorization: Bearer token123\r\n"
      "\r\n";

  ASSERT_PARSE_SUCCESS(parser, delete_request, request);

  assert(request.getMethod() == "DELETE");
  assert(request.getMethodCode() == HttpMethod::DELETE);
  assert(request.getRequestTarget() == "/api/users/123");
  assert(request.getHttpVersion() == "HTTP/1.1");
  assert(request.hasHeader("host"));
  assert(request.getHeader("host") == "api.example.com");
  assert(request.hasHeader("authorization"));
  assert(request.getHeader("authorization") == "Bearer token123");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_multiple_headers() {
  std::cout << "Testing request with headers..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string multi_header_request =
      "GET /api/data HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: Mozilla/5.0\r\n"
      "Accept: application/json\r\n"
      "Accept-Language: en-US,en;q=0.9\r\n"
      "Connection: keep-alive\r\n"
      "\r\n";

  ASSERT_PARSE_SUCCESS(parser, multi_header_request, request);

  assert(request.hasHeader("host"));
  assert(request.hasHeader("user-agent"));
  assert(request.hasHeader("accept"));
  assert(request.hasHeader("accept-language"));
  assert(request.hasHeader("connection"));

  assert(request.getHeader("host") == "example.com");
  assert(request.getHeader("user-agent") == "Mozilla/5.0");
  assert(request.getHeader("accept") == "application/json");
  assert(request.getHeader("accept-language") == "en-US,en;q=0.9");
  assert(request.getHeader("connection") == "keep-alive");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_empty_request_error() {
  std::cout << "Testing empty request error..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string empty_request = "";

  ASSERT_PARSE_ERROR(parser, empty_request, request);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_missing_crlf_error() {
  std::cout << "Testing missing CRLF error..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string no_crlf_request = "GET /index.html HTTP/1.1\nHost: example.com\n";

  ASSERT_PARSE_ERROR(parser, no_crlf_request, request);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_malformed_request_line() {
  std::cout << "Testing malformed request line..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  // Missing spaces
  std::string no_spaces = "GET/index.htmlHTTP/1.1\r\n\r\n";
  ASSERT_PARSE_ERROR(parser, no_spaces, request);

  // Missing target
  std::string no_target = "GET HTTP/1.1\r\n\r\n";
  ASSERT_PARSE_ERROR(parser, no_target, request);

  // Missing leading / target
  std::string wrong_target = "GET tours HTTP/1.1\r\n\r\n";
  ASSERT_PARSE_ERROR(parser, wrong_target, request);

  // Missing version
  std::string no_version = "GET /index.html\r\n\r\n";
  ASSERT_PARSE_ERROR(parser, no_version, request);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_unknown_method_error() {
  std::cout << "Testing unknown method error..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string unknown_method =
      "PATCH /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  ASSERT_PARSE_ERROR(parser, unknown_method, request);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_invalid_http_version() {
  std::cout << "Testing invalid HTTP version..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string invalid_format =
      "GET /index.html HTTP1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_ERROR(parser, invalid_format, request);

  std::string unsupported_version =
      "GET /index.html HTTP/2.0\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_ERROR(parser, unsupported_version, request);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_request_target_validation() {
  std::cout << "Testing request target validation..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string valid_path =
      "GET /path/to/resource HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, valid_path, request);

  std::string valid_query =
      "GET /search?q=test&limit=10 HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, valid_query, request);

  std::string valid_http_url =
      "GET http://example.com/resource HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, valid_http_url, request);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_edge_cases() {
  std::cout << "Testing edge cases..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  // Request without body but with headers
  std::string no_body =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: TestAgent\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, no_body, request);
  assert(request.getBody().empty());

  // Request with empty body
  std::string empty_body =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: 0\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, empty_body, request);
  assert(request.getBody().empty());

  // Minimum valid request
  std::string minimal =
      "GET / HTTP/1.1\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, minimal, request);
  assert(request.getRequestTarget() == "/");

  std::cout << "\t\t\t✓ passed" << std::endl;
}

static void test_http_version_support() {
  std::cout << "Testing HTTP version support..." << std::flush;

  HttpRequestParser parser;
  HttpRequest request;

  std::string http10 =
      "GET /index.html HTTP/1.0\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, http10, request);
  assert(request.getHttpVersion() == "HTTP/1.0");

  std::string http11 =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ASSERT_PARSE_SUCCESS(parser, http11, request);
  assert(request.getHttpVersion() == "HTTP/1.1");

  std::cout << "\t\t✓ passed" << std::endl;
}

void run_http_request_parser_tests() {
  std::cout << "=== Running HttpRequestParser Tests ===\n" << std::endl;

  test_basic_get_request();
  test_invalid_get_request();
  test_post_request_with_body();
  test_delete_request();
  test_multiple_headers();
  test_empty_request_error();
  test_missing_crlf_error();
  test_malformed_request_line();
  test_unknown_method_error();
  test_invalid_http_version();
  test_request_target_validation();
  test_http_version_support();
  test_edge_cases();

  std::cout << "\nAll HttpRequestParser tests passed!\n" << std::endl;
}
