#include <cassert>
#include <iostream>
#include <string>

#include "HttpMethodHandler.hpp"
#include "config.hpp"

// Test helper class to access protected methods
class HttpMethodHandlerTest : public HttpMethodHandler {
 public:
  HttpMethodHandlerTest(const ConfigParser::ServerConfig& config)
      : HttpMethodHandler(config) {}

  using HttpMethodHandler::getFilePath;
  using HttpMethodHandler::getLocation;
  using HttpMethodHandler::isFilePathSecure;
  using HttpMethodHandler::isMethodAllowed;
};

static ConfigParser::ServerConfig createTestConfig() {
  ConfigParser::Config config =
      ConfigParser::parse("tests/test-configs/test.conf");
  return config.servers.at(0);
}

static void test_constructor(ConfigParser::ServerConfig& config) {
  std::cout << "Testing HttpMethodHandler constructor" << std::flush;
  HttpMethodHandlerTest handler(config);
  std::cout << "\t✓ passed" << std::endl;
}

static void test_getLocation(ConfigParser::ServerConfig& config) {
  std::cout << "Testing getLocation method..." << std::flush;

  HttpMethodHandlerTest handler(config);

  // Test exact matches
  auto location = handler.getLocation("/");
  assert(location != nullptr);
  assert(location->path == "/");

  location = handler.getLocation("/tours");
  assert(location != nullptr);
  assert(location->path == "/tours");

  location = handler.getLocation("/red");
  assert(location != nullptr);
  assert(location->path == "/red");

  location = handler.getLocation("/tours/file.html");
  assert(location != nullptr);
  assert(location->path == "/tours");

  location = handler.getLocation("/tours/subfolder/index.html");
  assert(location != nullptr);
  assert(location->path == "/tours");

  location = handler.getLocation("/tours?param=value");
  assert(location != nullptr);
  assert(location->path == "/tours");

  // fallback to root location
  location = handler.getLocation("/nonexistent");
  assert(location != nullptr);
  assert(location->path == "/");

  location = handler.getLocation("/some/deep/path");
  assert(location != nullptr);
  assert(location->path == "/");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_getFilePath(ConfigParser::ServerConfig& config) {
  std::cout << "Testing getFilePath method..." << std::flush;

  HttpMethodHandlerTest handler(config);

  auto location = handler.getLocation("/index.html");
  std::string result = handler.getFilePath(*location, "/index.html");
  assert(result == "docs/fusion_web/index.html");

  result = handler.getFilePath(*location, "/");
  assert(result == "docs/fusion_web/");

  result = handler.getFilePath(*location, "/css/style.css");
  assert(result == "docs/fusion_web/css/style.css");

  location = handler.getLocation("/tours/tours1.html");
  result = handler.getFilePath(*location, "/tours/tours1.html");
  assert(result == "docs/fusion_web/tours/tours1.html");

  result = handler.getFilePath(*location, "/tours/subfolder/file.html");
  assert(result == "docs/fusion_web/tours/subfolder/file.html");

  // cgi-bin location with different root
  location = handler.getLocation("/test/script.py");
  result = handler.getFilePath(*location, "/test/script.py");
  assert(result == "./test/script.py");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_isFilePathSecure(ConfigParser::ServerConfig& config) {
  std::cout << "Testing isFilePathSecure method..." << std::flush;

  HttpMethodHandlerTest handler(config);
  std::string message;

  bool result = handler.isFilePathSecure("docs/fusion_web/index.html",
                                         "docs/fusion_web", message);
  assert(result == true);
  assert(message.empty());

  result = handler.isFilePathSecure("docs/fusion_web/subdir/file.txt",
                                    "docs/fusion_web", message);
  assert(result == true);
  assert(message.empty());

  result = handler.isFilePathSecure("./uploads/file.txt", "./", message);
  assert(result == true);
  assert(message.empty());

  result = handler.isFilePathSecure("/var/www/html/file.txt", "/var/www/html/",
                                    message);
  assert(result == true);
  assert(message.empty());

  // directory traversal attacks
  result = handler.isFilePathSecure("docs/fusion_web/../../../etc/passwd",
                                    "docs/fusion_web", message);
  assert(result == false);
  assert(!message.empty());
  message.clear();

  message.clear();
  result = handler.isFilePathSecure("docs/fusion_web/../config.txt",
                                    "docs/fusion_web", message);
  assert(result == false);
  assert(!message.empty());
  message.clear();

  result = handler.isFilePathSecure("", "/var/www/html", message);
  assert(result == false);
  assert(!message.empty());
  message.clear();

  std::cout << "\t✓ passed" << std::endl;
}

static void test_isMethodAllowed(ConfigParser::ServerConfig& config) {
  std::cout << "Testing isMethodAllowed method..." << std::flush;

  HttpMethodHandlerTest handler(config);

  // location /
  // allowed DELETE POST GET
  ConfigParser::LocationConfig* location = &config.locations[0];

  assert(handler.isMethodAllowed(*location, "GET") == true);
  assert(handler.isMethodAllowed(*location, "POST") == true);
  assert(handler.isMethodAllowed(*location, "DELETE") == true);
  assert(handler.isMethodAllowed(*location, "PUT") == false);
  assert(handler.isMethodAllowed(*location, "PATCH") == false);

  // location /test
  location = &config.locations[3];
  assert(location->allowed_methods.empty());
  assert(handler.isMethodAllowed(*location, "GET") == true);
  assert(handler.isMethodAllowed(*location, "POST") == true);
  assert(handler.isMethodAllowed(*location, "DELETE") == true);
  assert(handler.isMethodAllowed(*location, "PUT") == true);
  assert(handler.isMethodAllowed(*location, "PATCH") == true);

  std::cout << "\t✓ passed" << std::endl;
}

void run_http_method_handler_tests() {
  std::cout << "=== Running HttpMethodHandler Tests ===\n" << std::endl;

  ConfigParser::ServerConfig config = createTestConfig();

  test_constructor(config);
  test_getLocation(config);
  test_getFilePath(config);
  test_isFilePathSecure(config);
  test_isMethodAllowed(config);

  std::cout << "\nAll HttpMethodHandler tests passed!\n" << std::endl;
}
