#include <cassert>
#include <iostream>
#include <string>

#include "HttpResponse.hpp"
#include "HttpUtils.hpp"

static void test_constructor() {
  std::cout << "Testing HttpResponse constructor..." << std::flush;

  HttpResponse response;

  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::OK);
  assert(response.getHttpVersion() == "HTTP/1.1");
  assert(response.getReasonPhrase() == "OK");
  // assert(response.hasHeader("content-length"));

  std::cout << "\t✓ passed" << std::endl;
}

static void test_constructor_with_status() {
  std::cout << "Testing constructor with status code..." << std::flush;

  HttpResponse response(HttpUtils::HttpStatusCode::NOT_FOUND,
                        "Resource not found");

  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::NOT_FOUND);
  assert(response.getReasonPhrase() == "Not Found");
  assert(response.getBody().find("Resource not found") != std::string::npos);

  HttpResponse error_response(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Server error");

  assert(error_response.getStatusCode() ==
         HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR);
  assert(error_response.getBody().find("Server error") != std::string::npos);
  assert(error_response.getBody().find("error") != std::string::npos);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_unknown_status() {
  std::cout << "Testing unknown status code..." << std::flush;

  HttpResponse response(HttpUtils::HttpStatusCode::UKNOWN, "Unknown error");

  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::I_AM_TEAPOD);
  assert(response.getReasonPhrase() == "I'm a teapot");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_http_version_operations() {
  std::cout << "Testing HTTP version operations..." << std::flush;

  HttpResponse response;

  response.setHttpVersion("HTTP/2.0");
  assert(response.getHttpVersion() == "HTTP/2.0");

  response.setHttpVersion("HTTP/1.0");
  assert(response.getHttpVersion() == "HTTP/1.0");

  std::cout << "\t✓ passed" << std::endl;
}

static void test_status_code_operations() {
  std::cout << "Testing status code operations..." << std::flush;

  HttpResponse response;

  response.setStatusCode(HttpUtils::HttpStatusCode::CREATED);
  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::CREATED);

  response.setStatusCode(HttpUtils::HttpStatusCode::BAD_REQUEST);
  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::BAD_REQUEST);

  // Test unknown status code
  response.setStatusCode(HttpUtils::HttpStatusCode::UKNOWN);
  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::I_AM_TEAPOD);
  assert(response.getBody().find("uknown error") != std::string::npos);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_header_operations() {
  std::cout << "Testing header operations..." << std::flush;

  HttpResponse response;
  std::string content_type = "application/json";
  std::string server = "webserv/1.0";

  // Insert headers
  response.insertHeader("Content-Type", content_type);
  response.insertHeader("Server", server);

  // Test hasHeader (case insensitive)
  assert(response.hasHeader("Content-Type"));
  assert(response.hasHeader("content-type"));
  assert(response.hasHeader("CONTENT-TYPE"));
  assert(response.hasHeader("Server"));
  assert(!response.hasHeader("Authorization"));

  // Test getHeader (case insensitive)
  assert(response.getHeader("Content-Type") == "application/json");
  assert(response.getHeader("content-type") == "application/json");
  assert(response.getHeader("CONTENT-TYPE") == "application/json");
  assert(response.getHeader("Server") == "webserv/1.0");
  assert(response.getHeader("NonExistent").empty());

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_header_multiple_values() {
  std::cout << "Testing multiple header values..." << std::flush;

  HttpResponse response;

  response.insertHeader("Accept", "text/html");
  response.insertHeader("Accept", "application/json");

  std::string accept_header = response.getHeader("Accept");
  assert(accept_header.find("text/html") != std::string::npos);
  assert(accept_header.find("application/json") != std::string::npos);
  assert(accept_header.find(",") != std::string::npos);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_body_operations() {
  std::cout << "Testing body operations..." << std::flush;

  HttpResponse response;

  std::string test_body = "Hello, World!";
  response.setBody(test_body);
  assert(response.getBody() == test_body);

  // Content-Length should be updated automatically
  std::string content_length = response.getHeader("Content-Length");
  assert(content_length == std::to_string(test_body.length()));

  // Test empty body
  response.setBody("");
  assert(response.getBody().empty());
  assert(response.getHeader("Content-Length") == "0");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_convert_to_string() {
  std::cout << "Testing convert to string..." << std::flush;

  HttpResponse response;
  response.setBody("Test body");
  response.insertHeader("Content-Type", "text/plain");
  response.insertHeader("Server", "webserv/1.0");

  std::string http_string = response.convertToString();

  assert(http_string.find("HTTP/1.1 200 OK") != std::string::npos);
  assert(http_string.find("Content-Length:") != std::string::npos);
  assert(http_string.find("Content-Type: text/plain") != std::string::npos);
  assert(http_string.find("Server: webserv/1.0") != std::string::npos);
  assert(http_string.find("Test body") != std::string::npos);
  assert(http_string.find("\r\n\r\n") != std::string::npos);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_header_capitalization() {
  std::cout << "Testing header capitalization..." << std::flush;

  HttpResponse response;
  response.insertHeader("content-type", "application/json");
  response.insertHeader("x-custom-header", "value");

  std::string http_string = response.convertToString();

  assert(http_string.find("Content-Type: application/json") !=
         std::string::npos);
  assert(http_string.find("X-Custom-Header: value") != std::string::npos);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_error_responses() {
  std::cout << "Testing error responses..." << std::flush;

  HttpResponse response(HttpUtils::HttpStatusCode::BAD_REQUEST,
                        "Invalid syntax");

  std::string body = response.getBody();
  assert(body.find("error") != std::string::npos);
  assert(body.find("Invalid syntax") != std::string::npos);

  std::string http_string = response.convertToString();
  assert(http_string.find("400 Bad Request") != std::string::npos);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_teapot_response() {
  std::cout << "Testing teapot response..." << std::flush;

  HttpResponse response(HttpUtils::HttpStatusCode::I_AM_TEAPOD);

  assert(response.getStatusCode() == HttpUtils::HttpStatusCode::I_AM_TEAPOD);
  assert(response.getReasonPhrase() == "I'm a teapot");

  std::string http_string = response.convertToString();
  assert(http_string.find("418 I'm a teapot") != std::string::npos);

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_multiple_status_codes() {
  std::cout << "Testing multiple status codes..." << std::flush;

  // Test various status codes
  HttpResponse continue_response(HttpUtils::HttpStatusCode::CONTINUE);
  assert(continue_response.getReasonPhrase() == "Continue");
  assert(continue_response.convertToString().find("100") != std::string::npos);

  HttpResponse created_response(HttpUtils::HttpStatusCode::CREATED);
  assert(created_response.getReasonPhrase() == "Created");
  assert(created_response.convertToString().find("201") != std::string::npos);

  HttpResponse moved_response(HttpUtils::HttpStatusCode::MOVED_PERMANENTLY);
  assert(moved_response.getReasonPhrase() == "Moved Permanently");
  assert(moved_response.convertToString().find("301") != std::string::npos);

  HttpResponse unauthorized_response(HttpUtils::HttpStatusCode::UNAUTHORIZED);
  assert(unauthorized_response.getReasonPhrase() == "Unauthorized");
  assert(unauthorized_response.convertToString().find("401") !=
         std::string::npos);

  HttpResponse server_error_response(
      HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR);
  assert(server_error_response.getReasonPhrase() == "Internal Server Error");
  assert(server_error_response.convertToString().find("500") !=
         std::string::npos);

  std::cout << "\t✓ passed" << std::endl;
}

static void test_content_length_calculation() {
  std::cout << "Testing content length calculation..." << std::flush;

  HttpResponse response;

  // Test empty body
  response.setBody("");
  assert(response.getHeader("Content-Length") == "0");

  // Test short body
  response.setBody("Hi");
  assert(response.getHeader("Content-Length") == "2");

  // Test longer body
  std::string long_body(1000, 'x');
  response.setBody(long_body);
  assert(response.getHeader("Content-Length") == "1000");

  std::cout << "\t✓ passed" << std::endl;
}

static void test_header_overwriting() {
  std::cout << "Testing header overwriting..." << std::flush;

  HttpResponse response;
  response.insertHeader("Custom-Header", "value1");
  assert(response.getHeader("Custom-Header") == "value1");

  response.insertHeader("Custom-Header", "value2");
  std::string header_value = response.getHeader("Custom-Header");
  assert(header_value.find("value1") != std::string::npos);
  assert(header_value.find("value2") != std::string::npos);
  assert(header_value.find(",") != std::string::npos);

  std::cout << "\t\t✓ passed" << std::endl;
}

void run_http_response_tests() {
  std::cout << "=== Running HttpResponse Tests ===\n" << std::endl;

  test_constructor();
  test_constructor_with_status();
  test_unknown_status();
  test_http_version_operations();
  test_status_code_operations();
  test_header_operations();
  test_header_multiple_values();
  test_body_operations();
  test_convert_to_string();
  test_header_capitalization();
  test_error_responses();
  test_teapot_response();
  test_multiple_status_codes();
  test_content_length_calculation();
  test_header_overwriting();

  std::cout << "\nAll HttpResponse tests passed!\n" << std::endl;
}
