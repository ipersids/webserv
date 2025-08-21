/**
 * @file HttpMethodHandler.cpp
 * @brief Handles HTTP method processing for web server requests
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-14
 * @version 1.0
 *
 */

#include "HttpMethodHandler.hpp"

// constructor

/**
 * @brief Constructs an HttpMethodHandler with the given server configuration
 * @param config Reference to the server configuration object
 *
 * @note The configuration reference must remain valid for the lifetime
 *       of this HttpMethodHandler instance.
 */
HttpMethodHandler::HttpMethodHandler(const ConfigParser::ServerConfig& config)
    : _config(config) {}

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
HttpResponse HttpMethodHandler::processMethod(const HttpRequest& request) {
  HttpResponse response;
  const std::string uri = request.getRequestTarget();

  // find the longest match of location (config) for given target URI
  const ConfigParser::LocationConfig* location =
      HttpUtils::getLocation(uri, _config);
  if (!location) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND,
                              "Requested location not found");
    Logger::error("Requested location not found: " + uri);
    return response;
  }

  if (request.getBodyLength() >= location->client_max_body_size) {
    std::ostringstream error_msg;
    error_msg << "Request body size (" << request.getBodyLength()
              << " bytes) exceeds limit (" << location->client_max_body_size
              << " bytes) for " << request.getMethod() << " request";
    response.setErrorResponse(HttpUtils::HttpStatusCode::PAYLOAD_TOO_LARGE,
                              error_msg.str());
    Logger::error(error_msg.str());
    return response;
  }

  // check if this location requires redirection to another one
  if (!location->redirect_url.empty()) {
    response.setStatusCode(HttpUtils::HttpStatusCode::MOVED_PERMANENTLY);
    response.setBody("Redirecting to " + location->redirect_url);
    response.insertHeader("Location", location->redirect_url);
    response.insertHeader("Content-Type", "text/html");
    Logger::warning("Redirecting " + uri + " to " + location->redirect_url);
    return response;
  }

  // find full path to requested target URI
  const std::string file_path = HttpUtils::getFilePath(*location, uri);
  std::string message = "";
  if (!HttpUtils::isFilePathSecure(file_path, location->root, message)) {
    Logger::warning("Possible security problem " + uri);
    Logger::error("Failed to resolve uri " + uri + ": " + message);
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
                              "File not found");
    Logger::error("Requested file doesn't exist: " + path);
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
                              "Access denied");
    Logger::error("Directory access denied: " + path);
    return response;
  }

  // handle requested file
  if (std::filesystem::is_regular_file(path)) {
    Logger::info("Serving file: " + path);
    response = serveStaticFile(path);
  } else {
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Access denied");
    Logger::error("File access denied: " + path);
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
                              "Upload directory not found");
    Logger::error("Upload directory doesn't exist: " + path);
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
    Logger::error("Uploaded content type is not allowed: " + extension);
    return response;
  }

  // try to upload file
  std::string file_name = generateFileName(extension);
  std::string error_msg = "";
  if (!saveUploadedFile(path, file_name, request.getBody(), error_msg)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Failed to upload file to " + path);
    Logger::error("Failed to upload file to" + path);
    return response;
  }

  response.setStatusCode(HttpUtils::HttpStatusCode::CREATED);
  response.insertHeader("Content-Length", "0");
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
                              "File not found");
    Logger::error("Requested file doesn't exist: " + path);
    return response;
  }

  // reject deletion if it is directory
  if (std::filesystem::is_directory(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::CONFLICT,
                              "Deletion of directory is not allowed");
    Logger::error("Rejected to delete directory: " + path);
    return response;
  }

  // try to delete file
  if (!std::filesystem::remove(path)) {
    response.setErrorResponse(HttpUtils::HttpStatusCode::FORBIDDEN,
                              "Permission denied");
    Logger::error("Rejected to delete file: " + path);
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
                              "Access denied");
    return response;
  }
  response.setBody(body);
  response.setStatusCode(HttpUtils::HttpStatusCode::OK);
  response.insertHeader("Content-Type", HttpUtils::getMIME(path));
  Logger::info("Served file: " + path + " (" +
               response.getHeader("Content-Length") + " bytes)");
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
    response.insertHeader("Content-Type", "text/html");
    response.setBody(html.str());
    Logger::info("Generated directory listing for: " + path);
    return response;

  } catch (const std::exception& e) {
    Logger::error("Error listing directory " + path + ": " + e.what());
    response.setErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR,
                              "Internal server error");
  }

  return response;
}

HttpResponse HttpMethodHandler::handleMultipartFileUpload(
    const HttpRequest& request, const std::string& path,
    const std::string& content_type) {
  HttpResponse response;
  (void)path;
  (void)content_type;
  (void)request;
  return response;
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
    fs << content.c_str();
    fs.close();
  } catch (const std::exception& e) {
    error_msg = e.what();
    return false;
  }

  return true;
}

bool HttpMethodHandler::isAllowedFileType(const std::string& extention) {
  static const std::vector<std::string> allowed = {
      "txt", "pdf", "doc", "docx", "jpg", "jpeg", "png",
      "gif", "zip", "tar", "html", "css", "js",   "json"};

  for (const std::string& ext : allowed) {
    if (ext == extention) {
      return true;
    }
  }

  return false;
}

std::string HttpMethodHandler::generateFileName(std::string& extension) {
  std::time_t now = std::time(NULL);
  std::tm* ptm = std::localtime(&now);
  char buf[32];
  std::strftime(buf, 32, "%d.%m.%Y-%H%M%S", ptm);
  return std::string(buf) + "." + extension;
}
