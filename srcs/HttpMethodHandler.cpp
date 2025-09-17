/**
 * @file HttpMethodHandler.cpp
 * @brief Handles HTTP method processing for web server requests
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-14
 * @version 1.0
 *
 */

#include "HttpMethodHandler.hpp"

// public methods

/**
 * @brief Processes an HTTP request and returns the appropriate response
 *
 * This is the main entry point for HTTP method processing. It analyzes the
 * request method and routes it to the appropriate handler (GET, POST, DELETE).
 * The method also validates permissions and applies location-specific rules.
 *
 * @param request The HTTP request object containing method, URI, headers, and
 * body
 * @return HttpResponse object containing the complete response
 */
HttpResponse HttpMethodHandler::processMethod(
    const HttpRequest& request, const ConfigParser::ServerConfig& config) {
  HttpResponse response;
  const std::string uri = request.getRequestTarget();

  // find the longest match of location (config) for given target URI
  const ConfigParser::LocationConfig* location =
      HttpUtils::getLocation(uri, config);
  if (!location) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Requested location not found: " + uri);
    return response;
  }

  if (request.getBodyLength() >= location->client_max_body_size) {
    std::ostringstream error_msg;
    error_msg << "Request body size (" << request.getBodyLength()
              << " bytes) exceeds limit (" << location->client_max_body_size
              << " bytes) for " << request.getMethod() << " request";
    response.setErrorResponse(HttpUtils::HttpStatusCode::PAYLOAD_TOO_LARGE,
                              error_msg.str());
    return response;
  }

  // check if this location requires redirection to another one
  if (!location->redirect_url.empty()) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::MOVED_PERMANENTLY,
                              "Redirecting to " + location->redirect_url);
    response.insertHeader("Location", location->redirect_url);
    return response;
  }

  // find full path to requested target URI
  const std::string file_path = HttpUtils::getFilePath(*location, uri);
  std::string message = "";
  if (!HttpUtils::isFilePathSecure(file_path, location->root, message)) {
    Logger::warning("Possible security problem " + uri);
    Logger::warning("Failed to resolve uri " + uri + ": " + message);
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Page/file doesn't exist");
    return response;
  }

  // check if method is allowed by location in config file
  if (!HttpUtils::isMethodAllowed(*location, request.getMethod())) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::METHOD_NOT_ALLOWED,
                              "Method " + request.getMethod() + " not allowed");
    return response;
  }

  if (CgiHandler::isCgiRequest(file_path, *location)) {
    Logger::info("Processing CGI request :" + uri);
    return CgiHandler::execute(request, *location, file_path);
  }

  // perform method GET, POST or DELETE or give error
  const HttpMethod method_code = request.getMethodCode();
  switch (method_code) {
    case HttpMethod::GET:
      response = handleGetMethod(file_path, uri, *location);
      break;
    case HttpMethod::POST:
      response = handlePostMethod(file_path, request);
      break;
    case HttpMethod::DELETE:
      response = handleDeleteMethod(file_path);
      break;

    default:
      response.setErrorResponse(
          HttpUtils::HttpStatusCode::NOT_IMPLEMENTED,
          "Method " + request.getMethod() + " not implemented");
      break;
  }

  return response;
}

/// protected methods

/**
 * @brief Handles HTTP GET requests
 *
 * Processes GET requests by serving static files or generating directory
 * listings based on the requested path and location configuration.
 *
 * @param path The file system path to the requested resource
 * @param uri The original URI from the request
 * @param location The location configuration block that matches this request
 *
 * @return HttpResponse containing the file content or directory listing
 *
 * @see serveStaticFile()
 * @see serveDirectoryContent()
 */
HttpResponse HttpMethodHandler::handleGetMethod(
    const std::string& path, const std::string& uri,
    const ConfigParser::LocationConfig& location) {
  HttpResponse response;

  // check if file/directory exists
  if (!std::filesystem::exists(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "File not found: " + path);
    return response;
  }

  // check if it's a directory
  if (std::filesystem::is_directory(path)) {
    // check if we have default file to serve
    if (!location.index.empty()) {
      std::string index_path = path;
      if (!index_path.empty() && index_path.back() != '/') {
        index_path += '/';
      }
      index_path += location.index;
      if (std::filesystem::exists(index_path) &&
          std::filesystem::is_regular_file(index_path)) {
        Logger::info("Serving file: " + index_path);
        return serveStaticFile(index_path);
      }
    }

    // check if directory listing is allowed by config
    if (location.autoindex) {
      Logger::info("Listing directory: " + path);
      return serveDirectoryContent(path, uri);
    }

    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied: " + path);
    return response;
  }

  // handle requested file
  if (std::filesystem::is_regular_file(path)) {
    Logger::info("Serving file: " + path);
    response = serveStaticFile(path);
  } else {
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied: " + path);
  }

  return response;
}

/// @warning IT IS PLACEHOLDER, implementation needed
HttpResponse HttpMethodHandler::handlePostMethod(const std::string& path,
                                                 const HttpRequest& request) {
  HttpResponse response;

  // check if file/directory exists
  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Upload directory not found: " + path);
    return response;
  }

  // proccess file uploading depending on content type
  std::string content_type = request.getHeader("Content-Type");
  if (content_type.find("multipart/form-data") != std::string::npos) {
    return handleMultipartFileUpload(request, path, content_type);
  }

  std::string extension = HttpUtils::getExtension(content_type);
  if (!isAllowedFileType(extension)) {
    response.setErrorResponse(
        HttpUtils::HttpStatusCode::FORBIDDEN,
        "Uploaded content type is not allowed: " + content_type);
    return response;
  }

  // try to upload file
  std::string file_name = generateFileName(extension);
  std::string error_msg = "";
  if (!saveUploadedFile(path, file_name, request.getBody(), error_msg)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Failed to upload file to " + path);
    return response;
  }

  response.setStatusCode(HttpUtils::HttpStatusCode::CREATED);
  return response;
}

/**
 * @brief Handles HTTP DELETE requests for the specified file path.
 *
 * This method attempts to delete the file at the given path. If the path does
 * not exist, is a directory, or cannot be deleted due to permission issues, it
 * sets the appropriate error response in the returned HttpResponse object. Only
 * regular files can be deleted.
 *
 * @param path The file system path to the file to be deleted.
 * @return HttpResponse The HTTP response indicating the result of the delete
 * operation.
 *         - 204 No Content: File was successfully deleted.
 *         - 404 Not Found: File does not exist.
 *         - 409 Conflict: Path is a directory (deletion not allowed).
 *         - 403 Forbidden: File could not be deleted (e.g., permission denied).
 */
HttpResponse HttpMethodHandler::handleDeleteMethod(const std::string& path) {
  HttpResponse response;

  // check if file/directory exists
  if (!std::filesystem::exists(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "File not found: " + path);
    return response;
  }

  // reject deletion if it is directory
  if (std::filesystem::is_directory(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::CONFLICT,
                              "Deletion of directory is not allowed: " + path);
    return response;
  }

  // try to delete file
  if (!std::filesystem::remove(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Rejected to delete file: " + path);
    return response;
  }

  // set success response
  response.setStatusCode(HttpUtils::HttpStatusCode::NO_CONTENT);

  return response;
}

/// Helper functions

/**
 * @brief Serves a static file from the file system
 *
 * Reads a file from the file system and creates an HTTP response with
 * appropriate headers including Content-Type, Content-Length, and caching
 * headers.
 *
 * @param path The file system path to the file to serve
 * @return HttpResponse containing the file content and appropriate headers
 */
HttpResponse HttpMethodHandler::serveStaticFile(const std::string& path) {
  HttpResponse response;
  std::string body = "";

  if (HttpUtils::getFileContent(path, body) == -1) {
    Logger::error(body + ": " + path);
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied: " + path);
    return response;
  }
  response.setBody(body, HttpUtils::getMIME(path));
  response.setStatusCode(HttpUtils::HttpStatusCode::OK);
  return response;
}

/**
 * @brief Generates and serves directory listing content
 *
 * Creates an HTML directory listing page showing files and subdirectories
 * in the specified path. Only used when auto-index is enabled in the
 * location configuration.
 *
 * @param path The file system path to the directory to list
 * @param uri The original URI from the request (used for link generation)
 * @return HttpResponse containing HTML directory listing
 */
HttpResponse HttpMethodHandler::serveDirectoryContent(const std::string& path,
                                                      const std::string& uri) {
  HttpResponse response;

  try {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "<title>Index of " << uri << "</title>\n"
         << "</head>\n"
         << "<body>\n"
         << "<h1>Index of " << uri << "</h1>\n"
         << "<hr>\n"
         << "<ul>\n";

    if (uri != "/") {
      std::string parent_uri = uri;
      if (parent_uri.back() == '/') {
        parent_uri.pop_back();
      }
      size_t slash = parent_uri.find_last_of('/');
      if (slash != std::string::npos) {
        parent_uri = parent_uri.substr(0, slash + 1);
      } else {
        parent_uri = "/";
      }
      html << "<li><a href=\"" << parent_uri << "\">../</a></li>\n";
    }

    std::string link = uri;
    if (link.back() != '/') {
      link += '/';
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      std::string name = entry.path().filename().string();

      if (name[0] == '.') {
        continue;
      }

      if (entry.is_directory()) {
        html << "<li><a href=\"" << link << name << "/\">" << name
             << "/</a></li>\n";
      } else {
        html << "<li><a href=\"" << link << name << "\">" << name
             << "</a></li>\n";
      }
    }

    html << "</ul>\n"
         << "<hr>\n"
         << "<p><em>Hello from Webserv!</em></p>\n"
         << "</body>\n"
         << "</html>";

    response.setStatusCode(HttpUtils::HttpStatusCode::OK);
    response.setBody(html.str(), "text/html");
    return response;

  } catch (const std::exception& e) {
    Logger::warning("Error listing directory " + path + ": " + e.what());
    response.setErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Internal server error");
  }

  return response;
}

HttpResponse HttpMethodHandler::handleMultipartFileUpload(
    const HttpRequest& request, const std::string& path,
    const std::string& content_type) {
  HttpResponse response;
  std::vector<std::string> saved_files;

  std::string boundary = getMultipartBoundary(content_type);
  if (boundary.empty()) {
    response.setErrorResponse(
        HttpUtils::HttpStatusCode::BAD_REQUEST,
        "Missing or invalid boundary parameter in Content-Type");
    return response;
  }

  const std::string& body = request.getBody();
  boundary = "--" + boundary;

  size_t body_part_start = 0;
  size_t boundary_len = boundary.length();
  int file_count = 0;
  while ((body_part_start = body.find(boundary, body_part_start)) !=
         std::string::npos) {
    body_part_start += boundary_len;
    /// last boundary before prolog "--" + boundary + "--" ( -- delimiter)
    if (body.compare(body_part_start, 2, "--") == 0) {
      break;
    }

    if (body.compare(body_part_start, 2, "\r\n") == 0) {
      body_part_start += 2;
    }

    size_t header_end = body.find("\r\n\r\n", body_part_start);
    if (header_end == std::string::npos) {
      break;
    }

    std::string filename =
        getMultipartFileName(body, body_part_start, header_end);
    std::string extension =
        std::filesystem::path(filename).extension().string();
    if (extension.find('.') != std::string::npos) {
     extension = extension.substr(1, extension.length() - 1);
    }
    if (!isAllowedFileType(extension)) {
      response.setErrorResponse(
          HttpUtils::HttpStatusCode::FORBIDDEN,
          "Uploaded content type is not allowed: " + filename);
      return response;
    }

    if (filename.empty()) {
      body_part_start = header_end + 4;
      continue;
    }

    /// handle spaces in the file name
    std::transform(filename.begin(), filename.end(), filename.begin(),
                   [](unsigned char c) { return c == ' ' ? '-' : c; });

    size_t content_start = header_end + 4;
    size_t next_boundary = body.find(boundary, content_start);
    if (next_boundary == std::string::npos) {
      break;
    }

    size_t content_end = next_boundary;
    if (content_end >= 2 && body.compare(content_end - 2, 2, "\r\n") == 0) {
      content_end -= 2;
    } else if (content_end >= 1 && body[content_end] == '\n') {
      content_end -= 1;
    }

    std::string file_content =
        body.substr(content_start, content_end - content_start);

    std::string error_msg;
    if (!saveUploadedFile(path, filename, file_content, error_msg)) {
      response.setErrorResponse(
          HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
          "Failed to upload file: " + filename + " (" + error_msg + ")");
      return response;
    }
    Logger::info("Uploaded file: " + filename + " (" +
                 std::to_string(file_content.size()) + " bytes)");
    file_count += 1;
    body_part_start = next_boundary;
    saved_files.push_back(filename);
  }

  if (file_count == 0) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "No files found in multipart upload");
    return response;
  }

  response.setStatusCode(HttpUtils::HttpStatusCode::CREATED);
  response.setBody(generateUploadSuccessHtml(saved_files), "text/html");
  return response;
}

/// @see https://datatracker.ietf.org/doc/html/rfc2046#autoid-19
/// @see https://www.rfc-editor.org/rfc/rfc9112.html#name-field-syntax
/// field-line   = field-name ":" OWS field-value OWS
std::string HttpMethodHandler::getMultipartBoundary(
    const std::string& content_type) {
  size_t boundary_pos = content_type.find("boundary=");

  if (boundary_pos == std::string::npos) {
    return "";
  }

  /// handle quoted value
  size_t val_end_pos, val_start_pos = boundary_pos + 9;
  if (val_start_pos < content_type.length() &&
      content_type[val_start_pos] == '"') {
    val_start_pos += 1;
    val_end_pos = content_type.find("\"", val_start_pos);
    if (val_end_pos == std::string::npos) {
      return "";
    }
    return content_type.substr(val_start_pos, val_end_pos - val_start_pos);
  }

  val_end_pos = content_type.find_first_of(" \t;", val_start_pos);
  if (val_end_pos == std::string::npos) {
    val_end_pos = content_type.length();
  }

  return content_type.substr(val_start_pos, val_end_pos - val_start_pos);
}

std::string HttpMethodHandler::getMultipartFileName(const std::string& body,
                                                    size_t start, size_t end) {
  std::string headers = body.substr(start, end - start);

  /// Content-Disposition: form-data; name="field_name"; filename="file.txt"
  size_t content_disp_pos = headers.find("Content-Disposition:");
  if (content_disp_pos != std::string::npos) {
    /// filename presents in file upload case
    size_t file_name_pos = headers.find("filename=\"", content_disp_pos);
    if (file_name_pos != std::string::npos) {
      file_name_pos += 10;
      size_t file_name_end = headers.find("\"", file_name_pos);
      if (file_name_end != std::string::npos)
        return headers.substr(file_name_pos, file_name_end - file_name_pos);
    }
  }

  return "";
}

bool HttpMethodHandler::saveUploadedFile(const std::string& upload_dir,
                                         const std::string& file_name,
                                         const std::string& content,
                                         std::string& error_msg) {
  std::string full_path;
  if (!upload_dir.empty() && upload_dir.back() == '/')
    full_path = upload_dir + file_name;
  else
    full_path = upload_dir + "/" + file_name;

  if (std::filesystem::exists(full_path)) {
    error_msg = "File already exists";
    return false;
  }

  try {
    std::ofstream fs(full_path, std::ios::binary);
    if (!fs) {
      error_msg = "Failed to open file for writing: " + full_path;
      return false;
    }
    fs << content;
    fs.close();
  } catch (const std::exception& e) {
    error_msg = e.what();
    return false;
  }

  return true;
}

bool HttpMethodHandler::isAllowedFileType(const std::string& extension) {
  static const std::vector<std::string> allowed = {
      "txt", "pdf", "doc", "docx", "jpg", "jpeg", "png",
      "gif", "zip", "tar", "html", "css", "js",   "json"};

  for (const std::string& ext : allowed) {
    if (ext == extension) {
      return true;
    }
  }

  return false;
}

std::string HttpMethodHandler::generateFileName(const std::string& extension) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
      now.time_since_epoch()) % 100000;
  
  std::tm tm_buf;
  std::tm* ptm = localtime_r(&time_t, &tm_buf);
  char buf[48];
  std::strftime(buf, 32, "%d.%m.%Y-%H%M%S", ptm);
  
  std::string result(buf);
  result += "-" + std::to_string(microseconds.count());
  return result + "." + extension;
}

std::string HttpMethodHandler::generateUploadSuccessHtml(
    const std::vector<std::string>& files) {
  return "<html><head><link rel=\"stylesheet\" href=\"/style.css\"></head>"
         "<body><div class=\"hero\"><h1>Upload Success</h1>"
         "<p>Files: " +
         std::to_string(files.size()) +
         "</p>"
         "<a href=\"/\" class=\"cta-button\">Home</a></div></body></html>";
}
