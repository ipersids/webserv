/**
 * @file HttpUtils.cpp
 * @brief Implementation of utility functions for HTTP request processing
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 * @version 2.0
 *
 * This file contains the implementation of various utility functions used
 * throughout the HTTP server for common operations such as string manipulation,
 * file handling, security checks, and MIME type detection.
 *
 */

#include "HttpUtils.hpp"

/**
 * @brief Converts a string to lowercase
 * @param str The input string to convert to lowercase
 * @return A new string with all characters converted to lowercase
 *
 * @note The original string is not modified; a copy is returned
 */
std::string HttpUtils::toLowerCase(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

/**
 * @brief Finds the best matching location configuration for a given URI
 *
 * This function searches through all location configurations in the server
 * config and returns the one with the longest matching path prefix.
 *
 * @param request_uri The URI from the HTTP request to match against
 * @param config The server configuration containing location blocks
 * @return Pointer to the best matching LocationConfig, or nullptr if no match
 */
const ConfigParser::LocationConfig* HttpUtils::getLocation(
    const std::string& request_uri, const ConfigParser::ServerConfig& config) {
  const ConfigParser::LocationConfig* location = nullptr;
  size_t max_location_length = 0;

  for (const ConfigParser::LocationConfig& loc : config.locations) {
    if (request_uri.find(loc.path) == 0 &&
        loc.path.length() > max_location_length) {
      location = &loc;
      max_location_length = loc.path.length();
    }
  }

  return location;
}

/**
 * @brief Constructs the file system path for a given request URI
 *
 * This function combines the location's root directory with the request URI
 * to create the complete file system path.
 *
 * @param location The location configuration containing the root directory
 * @param request_uri The URI from the HTTP request
 * @return Complete file system path to the requested resource
 *
 * @note Automatically strips query parameters (everything after '?')
 */
const std::string HttpUtils::getFilePath(
    const ConfigParser::LocationConfig& location,
    const std::string& request_uri) {
  std::string path = location.root;

  if (!path.empty() && path.back() != '/') {
    path += '/';
  }

  size_t end = (request_uri.find('?') != std::string::npos)
                   ? request_uri.find('?')
                   : request_uri.length();
  if (request_uri[0] == '/') {
    path += request_uri.substr(1, end - 1);
  } else {
    path += request_uri.substr(0, end);
  }

  return path;
}

/**
 * @brief Reads the entire content of a file into a string
 * @param path The file system path to the file to read
 * @param body [out] String reference to store the file content
 * @return 0 on success, -1 on failure
 *
 * @note Opens file in binary mode to preserve exact content
 */
int HttpUtils::getFileContent(const std::string& path, std::string& body) {
  try {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
      body = "Failed read file";
      return -1;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    body = content;
  } catch (const std::exception& e) {
    body = e.what();
    return -1;
  }
  return 0;
}

/**
 * @brief Validates that a file path is secure and within allowed boundaries
 *
 * @param path The file path to validate
 * @param root The root directory that should contain the path
 * @param message [out] Error message if validation fails
 * @return true if the path is secure, false otherwise

 * @note Uses std::filesystem::weakly_canonical for safe path resolution
 * @note Prevents directory traversal attacks (../../../etc/passwd)
 * @note Handles symbolic links and relative path components
 */
bool HttpUtils::isFilePathSecure(const std::string& path,
                                 const std::string& root,
                                 std::string& message) {
  std::string secure_path;
  std::string secure_root;
  try {
    secure_path = std::filesystem::weakly_canonical(path);
  } catch (const std::exception& e) {
    message = e.what();
    return false;
  }
  try {
    secure_root = std::filesystem::weakly_canonical(root);
  } catch (const std::exception& e) {
    message = e.what();
    return false;
  }

  if (secure_path.find(secure_root) != 0) {
    message = "Security violation detected";
    return false;
  }

  return true;
}

/**
 * @brief Checks if an HTTP method is allowed for a specific location
 * @param location The location configuration to check against
 * @param method The HTTP method to validate (e.g., "GET", "POST", "DELETE")
 * @return true if the method is allowed, false otherwise
 */
bool HttpUtils::isMethodAllowed(const ConfigParser::LocationConfig& location,
                                const std::string& method) {
  if (location.allowed_methods.empty()) {
    return true;
  }

  return std::find(location.allowed_methods.begin(),
                   location.allowed_methods.end(),
                   method) != location.allowed_methods.end();
}

/**
 * @brief Determines the MIME type based on file extension
 *
 * This function analyzes the file extension of a given path and returns the
 * appropriate MIME type. It supports common web file types and defaults to
 * "application/octet-stream" for unknown extensions.
 *
 * @param path The file path or filename to analyze
 * @return The MIME type string corresponding to the file extension
 *
 * @note Extension matching is case-insensitive
 */
const std::string HttpUtils::getMIME(const std::string& path) {
  size_t dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return "application/octet-stream";
  }

  std::string extension = HttpUtils::toLowerCase(path.substr(dot_pos + 1));

  if (extension == "html" || extension == "htm") return "text/html";
  if (extension == "css") return "text/css";
  if (extension == "js") return "application/javascript";
  if (extension == "json") return "application/json";
  if (extension == "xml") return "application/xml";
  if (extension == "txt") return "text/plain";
  if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
  if (extension == "png") return "image/png";
  if (extension == "gif") return "image/gif";
  if (extension == "ico") return "image/x-icon";
  if (extension == "pdf") return "application/pdf";
  if (extension == "zip") return "application/zip";
  if (extension == "tar") return "application/x-tar";

  return "application/octet-stream";
}

bool HttpUtils::isRawRequestComplete(const std::string& raw_request) {
  // check end of headers (CRLFCRLF)
  size_t headers_end_pos = raw_request.find("\r\n\r\n");
  if (headers_end_pos == std::string::npos) {
    return false;
  }

  // check Content-Length header (case-insensitive)
  const std::string target = "content-length:";
  auto it =
      std::search(raw_request.begin(), raw_request.begin() + headers_end_pos,
                  target.begin(), target.end(), [](char a, char b) {
                    return std::tolower(static_cast<unsigned char>(a)) ==
                           std::tolower(static_cast<unsigned char>(b));
                  });

  if (it == raw_request.begin() + headers_end_pos) {
    return true;
  }

  size_t content_length_pos = std::distance(raw_request.begin(), it);
  size_t colon_pos = content_length_pos + target.length() - 1;
  size_t header_line_end = raw_request.find("\r\n", colon_pos);
  if (header_line_end == std::string::npos) {
    return false;
  }

  // parse the Content-Length value
  std::string value_str =
      raw_request.substr(colon_pos + 1, header_line_end - colon_pos - 1);
  size_t start = value_str.find_first_not_of(" \t");
  size_t end = value_str.find_last_not_of(" \t");
  if (start == std::string::npos) {
    return true;
  }
  value_str = value_str.substr(start, end - start + 1);

  try {
    size_t content_length = std::stoull(value_str);
    size_t received_body_size = raw_request.length() - (headers_end_pos + 4);
    return received_body_size >= content_length;
  } catch (const std::exception&) {
    return true;
  }
  return true;
}
