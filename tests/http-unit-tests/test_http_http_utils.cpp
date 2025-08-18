#include <cassert>
#include <iostream>
#include <string>

#include "HttpMethodHandler.hpp"
#include "HttpUtils.hpp"
#include "config.hpp"

// // Test helper class to access protected methods
// class HttpMethodHandlerTest : public HttpMethodHandler {
//  public:
//   HttpMethodHandlerTest(const ConfigParser::ServerConfig& config)
//       : HttpMethodHandler(config) {}

//   using HttpMethodHandler::getFilePath;
//   using HttpMethodHandler::getLocation;
//   using HttpMethodHandler::isFilePathSecure;
//   using HttpMethodHandler::isMethodAllowed;
// };

static ConfigParser::ServerConfig createTestConfig() {
  ConfigParser::Config config =
      ConfigParser::parse("tests/test-configs/test.conf");
  return config.servers.at(0);
}

static void test_getLocation(ConfigParser::ServerConfig& config) {
  std::cout << "Testing getLocation method..." << std::flush;

  // Test exact matches
  auto location = HttpUtils::getLocation("/", config);
  assert(location != nullptr);
  assert(location->path == "/");

  location = HttpUtils::getLocation("/tours", config);
  assert(location != nullptr);
  assert(location->path == "/tours");

  location = HttpUtils::getLocation("/red", config);
  assert(location != nullptr);
  assert(location->path == "/red");

  location = HttpUtils::getLocation("/tours/file.html", config);
  assert(location != nullptr);
  assert(location->path == "/tours");

  location = HttpUtils::getLocation("/tours/subfolder/index.html", config);
  assert(location != nullptr);
  assert(location->path == "/tours");

  location = HttpUtils::getLocation("/tours?param=value", config);
  assert(location != nullptr);
  assert(location->path == "/tours");

  // fallback to root location
  location = HttpUtils::getLocation("/nonexistent", config);
  assert(location != nullptr);
  assert(location->path == "/");

  location = HttpUtils::getLocation("/some/deep/path", config);
  assert(location != nullptr);
  assert(location->path == "/");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_getFilePath(ConfigParser::ServerConfig& config) {
  std::cout << "Testing getFilePath method..." << std::flush;

  auto location = HttpUtils::getLocation("/index.html", config);
  std::string result = HttpUtils::getFilePath(*location, "/index.html");
  assert(result == "docs/fusion_web/index.html");

  result = HttpUtils::getFilePath(*location, "/");
  assert(result == "docs/fusion_web/");

  result = HttpUtils::getFilePath(*location, "/css/style.css");
  assert(result == "docs/fusion_web/css/style.css");

  location = HttpUtils::getLocation("/tours/tours1.html", config);
  result = HttpUtils::getFilePath(*location, "/tours/tours1.html");
  assert(result == "docs/fusion_web/tours/tours1.html");

  result = HttpUtils::getFilePath(*location, "/tours/subfolder/file.html");
  assert(result == "docs/fusion_web/tours/subfolder/file.html");

  // cgi-bin location with different root
  location = HttpUtils::getLocation("/logs/webserv.log", config);
  result = HttpUtils::getFilePath(*location, "/logs/webserv.log");
  assert(result == "./logs/webserv.log");

  std::cout << "\t\t✓ passed" << std::endl;
}

static void test_isFilePathSecure() {
  std::cout << "Testing isFilePathSecure method..." << std::flush;

  std::string message;

  bool result = HttpUtils::isFilePathSecure("docs/fusion_web/index.html",
                                            "docs/fusion_web", message);
  assert(result == true);
  assert(message.empty());

  result = HttpUtils::isFilePathSecure("docs/fusion_web/subdir/file.txt",
                                       "docs/fusion_web", message);
  assert(result == true);
  assert(message.empty());

  result = HttpUtils::isFilePathSecure("./uploads/file.txt", "./", message);
  assert(result == true);
  assert(message.empty());

  result = HttpUtils::isFilePathSecure("/var/www/html/file.txt",
                                       "/var/www/html/", message);
  assert(result == true);
  assert(message.empty());

  // directory traversal attacks
  result = HttpUtils::isFilePathSecure("docs/fusion_web/../../../etc/passwd",
                                       "docs/fusion_web", message);
  assert(result == false);
  assert(!message.empty());
  message.clear();

  message.clear();
  result = HttpUtils::isFilePathSecure("docs/fusion_web/../config.txt",
                                       "docs/fusion_web", message);
  assert(result == false);
  assert(!message.empty());
  message.clear();

  result = HttpUtils::isFilePathSecure("", "/var/www/html", message);
  assert(result == false);
  assert(!message.empty());
  message.clear();

  std::cout << "\t✓ passed" << std::endl;
}

static void test_isMethodAllowed(ConfigParser::ServerConfig& config) {
  std::cout << "Testing isMethodAllowed method..." << std::flush;

  // location /
  // allowed DELETE POST GET
  ConfigParser::LocationConfig* location = &config.locations[0];

  assert(HttpUtils::isMethodAllowed(*location, "GET") == true);
  assert(HttpUtils::isMethodAllowed(*location, "POST") == true);
  assert(HttpUtils::isMethodAllowed(*location, "DELETE") == true);
  assert(HttpUtils::isMethodAllowed(*location, "PUT") == false);
  assert(HttpUtils::isMethodAllowed(*location, "PATCH") == false);

  // location /test
  location = &config.locations[3];
  assert(location->allowed_methods.empty());
  assert(HttpUtils::isMethodAllowed(*location, "GET") == true);
  assert(HttpUtils::isMethodAllowed(*location, "POST") == true);
  assert(HttpUtils::isMethodAllowed(*location, "DELETE") == true);
  assert(HttpUtils::isMethodAllowed(*location, "PUT") == true);
  assert(HttpUtils::isMethodAllowed(*location, "PATCH") == true);

  std::cout << "\t✓ passed" << std::endl;
}

void run_http_method_handler_tests() {
  std::cout << "=== Running HttpMethodHandler Tests ===\n" << std::endl;

  ConfigParser::ServerConfig config = createTestConfig();

  test_getLocation(config);
  test_getFilePath(config);
  test_isFilePathSecure();
  test_isMethodAllowed(config);

  std::cout << "\nAll HttpMethodHandler tests passed!\n" << std::endl;
}
