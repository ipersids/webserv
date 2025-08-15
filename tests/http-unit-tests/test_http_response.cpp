#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "../../includes/HttpResponse.hpp"
#include "../../includes/HttpUtils.hpp"

// Add ostream operator<< for HttpStatusCode for test output
inline std::ostream& operator<<(std::ostream& os,
                                HttpUtils::HttpStatusCode code) {
  return os << static_cast<int>(code);
}

// Simple test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name)                    \
  do {                                    \
    std::cout << "Testing " #name "... "; \
    test_##name();                        \
    std::cout << "PASSED\n";              \
  } while (0)

#define ASSERT_EQ(expected, actual)                                         \
  do {                                                                      \
    if ((expected) != (actual)) {                                           \
      std::cerr << "Assertion failed: " << #expected << " != " << #actual   \
                << " (expected: " << (expected) << ", actual: " << (actual) \
                << ")\n";                                                   \
      std::exit(1);                                                         \
    }                                                                       \
  } while (0)

#define ASSERT_TRUE(condition)                                             \
  do {                                                                     \
    if (!(condition)) {                                                    \
      std::cerr << "Assertion failed: " << #condition << " is not true\n"; \
      std::exit(1);                                                        \
    }                                                                      \
  } while (0)

#define ASSERT_FALSE(condition)                                             \
  do {                                                                      \
    if (condition) {                                                        \
      std::cerr << "Assertion failed: " << #condition << " is not false\n"; \
      std::exit(1);                                                         \
    }                                                                       \
  } while (0)

#define ASSERT_CONTAINS(haystack, needle)                                      \
  do {                                                                         \
    if ((haystack).find(needle) == std::string::npos) {                        \
      std::cerr << "Assertion failed: '" << haystack << "' does not contain '" \
                << needle << "'\n";                                            \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

// Test cases

TEST(default_constructor) {
  HttpResponse response;

  ASSERT_EQ(HttpUtils::HttpStatusCode::OK, response.getStatusCode());
  ASSERT_EQ("HTTP/1.1", response.getHttpVersion());
  ASSERT_EQ("OK", response.getReasonPhrase());
}

TEST(constructor_with_status_code) {
  HttpResponse response(HttpUtils::HttpStatusCode::NOT_FOUND,
                        "Resource not found");

  ASSERT_EQ(HttpUtils::HttpStatusCode::NOT_FOUND, response.getStatusCode());
  ASSERT_EQ("Not Found", response.getReasonPhrase());
  ASSERT_CONTAINS(response.getBody(), "Resource not found");
}

TEST(constructor_with_error_status) {
  HttpResponse response(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                        "Server error");

  ASSERT_EQ(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
            response.getStatusCode());
  ASSERT_CONTAINS(response.getBody(), "Server error");
  ASSERT_CONTAINS(response.getBody(), "error");
}

TEST(constructor_with_unknown_status) {
  HttpResponse response(HttpUtils::HttpStatusCode::UKNOWN, "Unknown error");

  ASSERT_EQ(HttpUtils::HttpStatusCode::I_AM_TEAPOD, response.getStatusCode());
  ASSERT_EQ("I'm a teapot", response.getReasonPhrase());
}

TEST(set_http_version) {
  HttpResponse response;
  response.setHttpVersion("HTTP/2.0");

  ASSERT_EQ("HTTP/2.0", response.getHttpVersion());
}

TEST(set_status_code) {
  HttpResponse response;
  response.setStatusCode(HttpUtils::HttpStatusCode::CREATED);

  ASSERT_EQ(HttpUtils::HttpStatusCode::CREATED, response.getStatusCode());
}

TEST(set_status_code_unknown) {
  HttpResponse response;
  response.setStatusCode(HttpUtils::HttpStatusCode::UKNOWN);

  ASSERT_EQ(HttpUtils::HttpStatusCode::I_AM_TEAPOD, response.getStatusCode());
  ASSERT_CONTAINS(response.getBody(), "uknown error");
}

TEST(insert_header) {
  HttpResponse response;
  response.insertHeader("Content-Type", "application/json");

  ASSERT_TRUE(response.hasHeader("content-type"));
  ASSERT_EQ("application/json", response.getHeader("Content-Type"));
}

TEST(insert_header_case_insensitive) {
  HttpResponse response;
  response.insertHeader("Content-Type", "application/json");

  ASSERT_TRUE(response.hasHeader("CONTENT-TYPE"));
  ASSERT_TRUE(response.hasHeader("content-type"));
  ASSERT_TRUE(response.hasHeader("Content-Type"));
  ASSERT_EQ("application/json", response.getHeader("content-TYPE"));
}

TEST(insert_header_multiple_values) {
  HttpResponse response;
  response.insertHeader("Accept", "text/html");
  response.insertHeader("Accept", "application/json");

  std::string accept_header = response.getHeader("Accept");
  ASSERT_CONTAINS(accept_header, "text/html");
  ASSERT_CONTAINS(accept_header, "application/json");
  ASSERT_CONTAINS(accept_header, ",");
}

TEST(set_body) {
  HttpResponse response;
  std::string test_body = "Hello, World!";
  response.setBody(test_body);

  ASSERT_EQ(test_body, response.getBody());
  // Content-Length should be updated automatically
  std::string content_length = response.getHeader("Content-Length");
  ASSERT_EQ(std::to_string(test_body.length()), content_length);
}

TEST(get_header_nonexistent) {
  HttpResponse response;
  std::string header_value = response.getHeader("NonExistent-Header");

  ASSERT_TRUE(header_value.empty());
  ASSERT_FALSE(response.hasHeader("NonExistent-Header"));
}

TEST(convert_to_string_basic) {
  HttpResponse response;
  response.setBody("Test body");

  std::string http_string = response.convertToString();

  ASSERT_CONTAINS(http_string, "HTTP/1.1 200 OK");
  ASSERT_CONTAINS(http_string, "Content-Length:");
  ASSERT_CONTAINS(http_string, "Test body");
  ASSERT_CONTAINS(http_string, "\r\n\r\n");  // Header-body separator
}

TEST(convert_to_string_with_headers) {
  HttpResponse response;
  response.insertHeader("Content-Type", "text/plain");
  response.insertHeader("Server", "webserv/1.0");
  response.setBody("Hello World");

  std::string http_string = response.convertToString();

  ASSERT_CONTAINS(http_string, "Content-Type: text/plain");
  ASSERT_CONTAINS(http_string, "Server: webserv/1.0");
  ASSERT_CONTAINS(http_string, "Content-Length:");
}

TEST(convert_to_string_header_capitalization) {
  HttpResponse response;
  response.insertHeader("content-type", "application/json");
  response.insertHeader("x-custom-header", "value");

  std::string http_string = response.convertToString();

  ASSERT_CONTAINS(http_string, "Content-Type: application/json");
  ASSERT_CONTAINS(http_string, "X-Custom-Header: value");
}

TEST(error_response_format) {
  HttpResponse response(HttpUtils::HttpStatusCode::BAD_REQUEST,
                        "Invalid syntax");

  std::string body = response.getBody();
  ASSERT_CONTAINS(body, "error");
  ASSERT_CONTAINS(body, "Invalid syntax");

  std::string http_string = response.convertToString();
  ASSERT_CONTAINS(http_string, "400 Bad Request");
}

TEST(teapot_response) {
  HttpResponse response(HttpUtils::HttpStatusCode::I_AM_TEAPOD);

  ASSERT_EQ(HttpUtils::HttpStatusCode::I_AM_TEAPOD, response.getStatusCode());
  ASSERT_EQ("I'm a teapot", response.getReasonPhrase());

  std::string http_string = response.convertToString();
  ASSERT_CONTAINS(http_string, "418 I'm a teapot");
}

TEST(multiple_status_codes) {
  struct {
    HttpUtils::HttpStatusCode code;
    const char* expected_phrase;
    int expected_number;
  } test_cases[] = {
      {HttpUtils::HttpStatusCode::CONTINUE, "Continue", 100},
      {HttpUtils::HttpStatusCode::CREATED, "Created", 201},
      {HttpUtils::HttpStatusCode::MOVED_PERMANENTLY, "Moved Permanently", 301},
      {HttpUtils::HttpStatusCode::UNAUTHORIZED, "Unauthorized", 401},
      {HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
       "Internal Server Error", 500}};

  for (const auto& test_case : test_cases) {
    HttpResponse response(test_case.code);
    ASSERT_EQ(test_case.code, response.getStatusCode());
    ASSERT_EQ(std::string(test_case.expected_phrase),
              response.getReasonPhrase());

    std::string http_string = response.convertToString();
    ASSERT_CONTAINS(http_string, std::to_string(test_case.expected_number));
    ASSERT_CONTAINS(http_string, test_case.expected_phrase);
  }
}

TEST(content_length_calculation) {
  HttpResponse response;

  // Test empty body
  response.setBody("");
  ASSERT_EQ("0", response.getHeader("Content-Length"));

  // Test short body
  response.setBody("Hi");
  ASSERT_EQ("2", response.getHeader("Content-Length"));

  // Test longer body
  std::string long_body(1000, 'x');
  response.setBody(long_body);
  ASSERT_EQ("1000", response.getHeader("Content-Length"));
}

TEST(header_overwriting) {
  HttpResponse response;
  response.insertHeader("Custom-Header", "value1");
  ASSERT_EQ("value1", response.getHeader("Custom-Header"));

  response.insertHeader("Custom-Header", "value2");
  std::string header_value = response.getHeader("Custom-Header");
  ASSERT_CONTAINS(header_value, "value1");
  ASSERT_CONTAINS(header_value, "value2");
  ASSERT_CONTAINS(header_value, ",");
}

// Main test runner
void run_http_response_tests() {
  std::cout << "Running HttpResponse tests...\n\n";

  try {
    RUN_TEST(default_constructor);
    RUN_TEST(constructor_with_status_code);
    RUN_TEST(constructor_with_error_status);
    RUN_TEST(constructor_with_unknown_status);
    RUN_TEST(set_http_version);
    RUN_TEST(set_status_code);
    RUN_TEST(set_status_code_unknown);
    RUN_TEST(insert_header);
    RUN_TEST(insert_header_case_insensitive);
    RUN_TEST(insert_header_multiple_values);
    RUN_TEST(set_body);
    RUN_TEST(get_header_nonexistent);
    RUN_TEST(convert_to_string_basic);
    RUN_TEST(convert_to_string_with_headers);
    RUN_TEST(convert_to_string_header_capitalization);
    RUN_TEST(error_response_format);
    RUN_TEST(teapot_response);
    RUN_TEST(multiple_status_codes);
    RUN_TEST(content_length_calculation);
    RUN_TEST(header_overwriting);

    std::cout << "\nAll tests passed! ✅\n";
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
  }
}
