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
 * appropriate MIME type. It supports a comprehensive list of web file types
 * and defaults to "application/octet-stream" for unknown extensions.
 *
 * @param path The file path or filename to analyze
 * @return The MIME type string corresponding to the file extension
 *
 * @note Extension matching is case-insensitive
 * @note Based on standard nginx MIME type mappings
 */
const std::string HttpUtils::getMIME(const std::string& path) {
  static const std::unordered_map<std::string, std::string> extension_to_mime =
      {{"html", "text/html"},
       {"htm", "text/html"},
       {"shtml", "text/html"},
       {"css", "text/css"},
       {"xml", "text/xml"},
       {"txt", "text/plain"},
       {"mml", "text/mathml"},
       {"jad", "text/vnd.sun.j2me.app-descriptor"},
       {"wml", "text/vnd.wap.wml"},
       {"htc", "text/x-component"},

       {"gif", "image/gif"},
       {"jpeg", "image/jpeg"},
       {"jpg", "image/jpeg"},
       {"png", "image/png"},
       {"tif", "image/tiff"},
       {"tiff", "image/tiff"},
       {"wbmp", "image/vnd.wap.wbmp"},
       {"ico", "image/x-icon"},
       {"jng", "image/x-jng"},
       {"bmp", "image/x-ms-bmp"},
       {"svg", "image/svg+xml"},
       {"svgz", "image/svg+xml"},
       {"webp", "image/webp"},

       {"js", "application/javascript"},
       {"atom", "application/atom+xml"},
       {"rss", "application/rss+xml"},
       {"woff", "application/font-woff"},
       {"jar", "application/java-archive"},
       {"war", "application/java-archive"},
       {"ear", "application/java-archive"},
       {"json", "application/json"},
       {"hqx", "application/mac-binhex40"},
       {"doc", "application/msword"},
       {"pdf", "application/pdf"},
       {"ps", "application/postscript"},
       {"eps", "application/postscript"},
       {"ai", "application/postscript"},
       {"rtf", "application/rtf"},
       {"m3u8", "application/vnd.apple.mpegurl"},
       {"xls", "application/vnd.ms-excel"},
       {"eot", "application/vnd.ms-fontobject"},
       {"ppt", "application/vnd.ms-powerpoint"},
       {"wmlc", "application/vnd.wap.wmlc"},
       {"kml", "application/vnd.google-earth.kml+xml"},
       {"kmz", "application/vnd.google-earth.kmz"},
       {"7z", "application/x-7z-compressed"},
       {"cco", "application/x-cocoa"},
       {"jardiff", "application/x-java-archive-diff"},
       {"jnlp", "application/x-java-jnlp-file"},
       {"run", "application/x-makeself"},
       {"pl", "application/x-perl"},
       {"pm", "application/x-perl"},
       {"prc", "application/x-pilot"},
       {"pdb", "application/x-pilot"},
       {"rar", "application/x-rar-compressed"},
       {"rpm", "application/x-redhat-package-manager"},
       {"sea", "application/x-sea"},
       {"swf", "application/x-shockwave-flash"},
       {"sit", "application/x-stuffit"},
       {"tcl", "application/x-tcl"},
       {"tk", "application/x-tcl"},
       {"der", "application/x-x509-ca-cert"},
       {"pem", "application/x-x509-ca-cert"},
       {"crt", "application/x-x509-ca-cert"},
       {"xpi", "application/x-xpinstall"},
       {"xhtml", "application/xhtml+xml"},
       {"xspf", "application/xspf+xml"},
       {"zip", "application/zip"},
       {"docx",
        "application/"
        "vnd.openxmlformats-officedocument.wordprocessingml.document"},
       {"xlsx",
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
       {"pptx",
        "application/"
        "vnd.openxmlformats-officedocument.presentationml.presentation"},

       {"mid", "audio/midi"},
       {"midi", "audio/midi"},
       {"kar", "audio/midi"},
       {"mp3", "audio/mpeg"},
       {"ogg", "audio/ogg"},
       {"m4a", "audio/x-m4a"},
       {"ra", "audio/x-realaudio"},

       {"3gpp", "video/3gpp"},
       {"3gp", "video/3gpp"},
       {"ts", "video/mp2t"},
       {"mp4", "video/mp4"},
       {"mpeg", "video/mpeg"},
       {"mpg", "video/mpeg"},
       {"mov", "video/quicktime"},
       {"webm", "video/webm"},
       {"flv", "video/x-flv"},
       {"m4v", "video/x-m4v"},
       {"mng", "video/x-mng"},
       {"asx", "video/x-ms-asf"},
       {"asf", "video/x-ms-asf"},
       {"wmv", "video/x-ms-wmv"},
       {"avi", "video/x-msvideo"},

       {"bin", "application/octet-stream"},
       {"exe", "application/octet-stream"},
       {"dll", "application/octet-stream"},
       {"deb", "application/octet-stream"},
       {"dmg", "application/octet-stream"},
       {"iso", "application/octet-stream"},
       {"img", "application/octet-stream"},
       {"msi", "application/octet-stream"},
       {"msp", "application/octet-stream"},
       {"msm", "application/octet-stream"}};

  size_t dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return "application/octet-stream";
  }

  std::string extension = HttpUtils::toLowerCase(path.substr(dot_pos + 1));

  std::unordered_map<std::string, std::string>::const_iterator it =
      extension_to_mime.find(extension);
  if (it != extension_to_mime.end()) {
    return it->second;
  }

  return "application/octet-stream";
}

/**
 * @brief Determines file extension based on the MIME type
 *
 * This function analyzes the MIME type and returns the most appropriate
 * file extension. It supports a comprehensive list of web file types and
 * defaults to empty string for unknown MIME types.
 *
 * @param content_type The MIME type string to analyze
 * @return The file extension (without dot) corresponding to the MIME type
 *
 * @note MIME type matching is case-insensitive
 * @note Returns the first/most common extension for each MIME type
 */
const std::string HttpUtils::getExtension(const std::string& content_type) {
  static const std::unordered_map<std::string, std::string> mime_to_extension =
      {
          {"text/html", "html"},
          {"text/css", "css"},
          {"text/xml", "xml"},
          {"text/plain", "txt"},
          {"text/mathml", "mml"},
          {"text/vnd.sun.j2me.app-descriptor", "jad"},
          {"text/vnd.wap.wml", "wml"},
          {"text/x-component", "htc"},

          {"image/gif", "gif"},
          {"image/jpeg", "jpeg"},
          {"image/png", "png"},
          {"image/tiff", "tiff"},
          {"image/vnd.wap.wbmp", "wbmp"},
          {"image/x-icon", "ico"},
          {"image/x-jng", "jng"},
          {"image/x-ms-bmp", "bmp"},
          {"image/svg+xml", "svg"},
          {"image/webp", "webp"},

          {"application/javascript", "js"},
          {"application/atom+xml", "atom"},
          {"application/rss+xml", "rss"},
          {"application/font-woff", "woff"},
          {"application/java-archive", "jar"},
          {"application/json", "json"},
          {"application/mac-binhex40", "hqx"},
          {"application/msword", "doc"},
          {"application/pdf", "pdf"},
          {"application/postscript", "ps"},
          {"application/rtf", "rtf"},
          {"application/vnd.apple.mpegurl", "m3u8"},
          {"application/vnd.ms-excel", "xls"},
          {"application/vnd.ms-fontobject", "eot"},
          {"application/vnd.ms-powerpoint", "ppt"},
          {"application/vnd.wap.wmlc", "wmlc"},
          {"application/vnd.google-earth.kml+xml", "kml"},
          {"application/vnd.google-earth.kmz", "kmz"},
          {"application/x-7z-compressed", "7z"},
          {"application/x-cocoa", "cco"},
          {"application/x-java-archive-diff", "jardiff"},
          {"application/x-java-jnlp-file", "jnlp"},
          {"application/x-makeself", "run"},
          {"application/x-perl", "pl"},
          {"application/x-pilot", "prc"},
          {"application/x-rar-compressed", "rar"},
          {"application/x-redhat-package-manager", "rpm"},
          {"application/x-sea", "sea"},
          {"application/x-shockwave-flash", "swf"},
          {"application/x-stuffit", "sit"},
          {"application/x-tcl", "tcl"},
          {"application/x-x509-ca-cert", "crt"},
          {"application/x-xpinstall", "xpi"},
          {"application/xhtml+xml", "xhtml"},
          {"application/xspf+xml", "xspf"},
          {"application/zip", "zip"},
          {"application/"
           "vnd.openxmlformats-officedocument.wordprocessingml.document",
           "docx"},
          {"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
           "xlsx"},
          {"application/"
           "vnd.openxmlformats-officedocument.presentationml.presentation",
           "pptx"},

          {"audio/midi", "midi"},
          {"audio/mpeg", "mp3"},
          {"audio/ogg", "ogg"},
          {"audio/x-m4a", "m4a"},
          {"audio/x-realaudio", "ra"},

          {"video/3gpp", "3gpp"},
          {"video/mp2t", "ts"},
          {"video/mp4", "mp4"},
          {"video/mpeg", "mpeg"},
          {"video/quicktime", "mov"},
          {"video/webm", "webm"},
          {"video/x-flv", "flv"},
          {"video/x-m4v", "m4v"},
          {"video/x-mng", "mng"},
          {"video/x-ms-asf", "asf"},
          {"video/x-ms-wmv", "wmv"},
          {"video/x-msvideo", "avi"},

          {"application/octet-stream", "bin"}
          // prefer bin over exe/dll/deb/etc.
      };

  std::string mime = HttpUtils::toLowerCase(content_type);

  std::unordered_map<std::string, std::string>::const_iterator it =
      mime_to_extension.find(mime);
  if (it != mime_to_extension.end()) {
    return it->second;
  }
  return "";
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
