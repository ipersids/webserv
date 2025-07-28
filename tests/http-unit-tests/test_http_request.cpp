#include <cassert>
#include <iostream>
#include <string>

#include "HttpRequest.hpp"

/// @todo duplicates in header fuild names....

static void test_constructor() {
  std::cout << "Testing HttpRequest constructor..." << std::flush;

  HttpRequest request;

  assert(request.getMethodCode() == HttpMethod::UNKNOWN);
  assert(request.getMethod().empty());
  assert(request.getRequestTarget().empty());
  assert(request.getHttpVersion().empty());
  assert(!request.hasHeader("Content-Type"));
  assert(request.getBody().empty());

  std::cout << "\t✓ passed" << std::endl;
}

static void test_method_operations() {
  std::cout << "Testing method operations..." << std::flush;

  HttpRequest request;

  request.setMethod("GET");
  assert(request.getMethod() == "GET");
  assert(request.getMethodCode() == HttpMethod::GET);

  request.setMethod("POST");
  assert(request.getMethod() == "POST");
  assert(request.getMethodCode() == HttpMethod::POST);

  request.setMethod("DELETE");
  assert(request.getMethod() == "DELETE");
  assert(request.getMethodCode() == HttpMethod::DELETE);

  request.setMethod("PATCH");
  assert(request.getMethod() == "PATCH");
  assert(request.getMethodCode() == HttpMethod::UNKNOWN);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_request_target() {
  std::cout << "Testing request target operations..." << std::flush;

  HttpRequest request;

  request.setRequestTarget("/index.html");
  assert(request.getRequestTarget() == "/index.html");

  request.setRequestTarget("/api/users?id=123");
  assert(request.getRequestTarget() == "/api/users?id=123");

  std::cout << "\t✓ passed" << std::endl;
}

static void test_http_version() {
  std::cout << "Testing HTTP version operations..." << std::flush;

  HttpRequest request;

  request.setHttpVersion("HTTP/1.1");
  assert(request.getHttpVersion() == "HTTP/1.1");

  request.setHttpVersion("HTTP/2.0");
  assert(request.getHttpVersion() == "HTTP/2.0");

  std::cout << "\t✓ passed" << std::endl;
}

static void test_header_operations() {
  std::cout << "Testing header operations..." << std::flush;

  HttpRequest request;
  std::string content_type = "application/json";
  std::string host = "localhost:8080";
  std::string user_agent = "Mozilla/5.0";

  // Insert headers
  request.insertHeader("Content-Type", content_type);
  request.insertHeader("Host", host);
  request.insertHeader("User-Agent", user_agent);

  // Test hasHeader (case insensitive)
  assert(request.hasHeader("Content-Type"));
  assert(request.hasHeader("content-type"));
  assert(request.hasHeader("CONTENT-TYPE"));
  assert(request.hasHeader("Host"));
  assert(request.hasHeader("User-Agent"));
  assert(!request.hasHeader("Authorization"));

  // Test getHeader (case insensitive)
  assert(request.getHeader("Content-Type") == "application/json");
  assert(request.getHeader("content-type") == "application/json");
  assert(request.getHeader("CONTENT-TYPE") == "application/json");
  assert(request.getHeader("Host") == "localhost:8080");
  assert(request.getHeader("User-Agent") == "Mozilla/5.0");
  assert(request.getHeader("NonExistent").empty());

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_body_operations() {
  std::cout << "Testing body operations..." << std::flush;

  HttpRequest request;

  std::string json_body = "{\"name\":\"John\",\"age\":30}";
  request.setBody(json_body);
  assert(request.getBody() == json_body);

  request.setBody("");
  assert(request.getBody().empty());

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_complete_request() {
  std::cout << "Testing complete HTTP request..." << std::flush;

  HttpRequest request;

  request.setMethod("POST");
  request.setRequestTarget("/api/users");
  request.setHttpVersion("HTTP/1.1");

  std::string content_type = "application/json";
  std::string host = "example.com";
  std::string content_length = "26";

  request.insertHeader("Content-Type", content_type);
  request.insertHeader("Host", host);
  request.insertHeader("Content-Length", content_length);

  request.setBody("{\"name\":\"Alice\",\"age\":25}");

  assert(request.getMethod() == "POST");
  assert(request.getMethodCode() == HttpMethod::POST);
  assert(request.getRequestTarget() == "/api/users");
  assert(request.getHttpVersion() == "HTTP/1.1");
  assert(request.getHeader("Content-Type") == "application/json");
  assert(request.getHeader("Host") == "example.com");
  assert(request.getHeader("Content-Length") == "26");
  assert(request.getBody() == "{\"name\":\"Alice\",\"age\":25}");

  std::cout << "\t✓ passed" << std::endl;
}

static void test_edge_cases() {
  std::cout << "Testing edge cases..." << std::flush;

  HttpRequest request;

  request.setMethod("");
  assert(request.getMethod().empty());
  assert(request.getMethodCode() == HttpMethod::UNKNOWN);

  request.setRequestTarget("");
  assert(request.getRequestTarget().empty());

  request.setHttpVersion("");
  assert(request.getHttpVersion().empty());

  // Test case sensitivity for methods (should be case-sensitive)
  request.setMethod("get");
  assert(request.getMethod() == "get");
  assert(request.getMethodCode() == HttpMethod::UNKNOWN);

  // Test header case insensitivity
  std::string mixed_case_value = "mixed-case";
  request.insertHeader("Mixed-Case-Header", mixed_case_value);
  assert(request.hasHeader("mixed-case-header"));
  assert(request.hasHeader("MIXED-CASE-HEADER"));
  assert(request.getHeader("mixed-case-header") == "mixed-case");

  std::cout << "\t\t\t✓ passed" << std::endl;
}

void run_http_request_tests() {
  std::cout << "=== Running HttpRequest Tests ===\n" << std::endl;

  test_constructor();
  test_method_operations();
  test_request_target();
  test_http_version();
  test_header_operations();
  test_body_operations();
  test_complete_request();
  test_edge_cases();

  std::cout << "\nAll HttpRequest tests passed!" << std::endl;
}
