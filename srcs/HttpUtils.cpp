/**
 * @file HttpUtils.cpp
 * @brief Implementation of utility functions for HTTP request processing
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 */

#include "HttpUtils.hpp"

std::string HttpUtils::toLowerCase(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

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

bool HttpUtils::isMethodAllowed(const ConfigParser::LocationConfig& location,
                                const std::string& method) {
  if (location.allowed_methods.empty()) {
    return true;
  }

  return std::find(location.allowed_methods.begin(),
                   location.allowed_methods.end(),
                   method) != location.allowed_methods.end();
}

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
