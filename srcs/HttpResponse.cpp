#include "HttpResponse.hpp"

HttpResponse::HttpResponse()
    : _status_code(HttpUtils::HttpStatusCode::I_AM_TEAPOD),
      _headers(),
      _body(""),
      _content_type(""),
      _is_error_response(false),
      _is_keep_alive_connection(true) {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
  if (this == &other) {
    return *this;
  }

  this->_body = other._body;
  this->_headers.clear();
  this->_headers = other._headers;
  this->_status_code = other._status_code;
  this->_content_type = other._content_type;
  this->_is_error_response = other._is_error_response;
  this->_is_keep_alive_connection = other._is_keep_alive_connection;

  return *this;
}

HttpResponse::HttpResponse(const HttpResponse& other) { *this = other; }

HttpResponse::~HttpResponse() { _headers.clear(); }

// Setters

void HttpResponse::setStatusCode(const HttpUtils::HttpStatusCode& code) {
  if (code == HttpUtils::HttpStatusCode::UKNOWN) {
    setStatusCode(HttpUtils::HttpStatusCode::I_AM_TEAPOD);
  } else {
    _status_code = code;
  }
}

void HttpResponse::insertHeader(const std::string& field_name,
                                const std::string& value) {
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
  auto it = _headers.find(lowercase_name);
  if (it != _headers.end()) {
    it->second = value;
  }
}

void HttpResponse::setBody(const std::string& body,
                           const std::string& content_type) {
  _body = body;
  _content_type = content_type;
}

void HttpResponse::setContentType(const std::string& content_type) {
  _content_type = content_type;
}

void HttpResponse::setConnectionHeader(
    const std::string& request_connection,
    const std::string& request_http_version) {
  if (_status_code == HttpUtils::HttpStatusCode::BAD_REQUEST ||
      _status_code == HttpUtils::HttpStatusCode::REQUEST_TIMEOUT ||
      _status_code >= HttpUtils::HttpStatusCode::LENGTH_REQUIRED ||
      request_connection == "close" ||
      (request_http_version == "HTTP/1.0" &&
       request_connection != "keep-alive")) {
    insertHeader("Connection", "close");
    _is_keep_alive_connection = false;
  } else {
    insertHeader("Connection", "keep-alive");
    _is_keep_alive_connection = true;
  }
}

void HttpResponse::setErrorResponse(const HttpUtils::HttpStatusCode& code,
                                    const std::string& message) {
  setStatusCode(code);
  setBody(message);
  _is_error_response = true;
}

void HttpResponse::setErrorPageBody(
    const ConfigParser::ServerConfig& server_config) {
  std::map<int, std::string>::const_iterator it =
      server_config.error_pages.find(static_cast<int>(_status_code));
  if (it == server_config.error_pages.end()) {
    setDefaultCatErrorPage();
    return;
  }

  std::string path = server_config.root;
  if (path.empty() || path.back() != '/') {
    path += "/";
  }
  if (it->second.empty() || it->second[0] != '/') {
    path += it->second;
  } else {
    path += ((it->second.length() >= 2) ? it->second.substr(1) : "");
  }

  std::string tmp_body = "";
  if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path) &&
      HttpUtils::getFileContent(path, tmp_body) == 0) {
    setBody(tmp_body, HttpUtils::getMIME(path));
  } else {
    setDefaultCatErrorPage();
  }
}

void HttpResponse::setDefaultCatErrorPage(void) {
  std::string cat_url =
      "https://http.cat/" + std::to_string(static_cast<int>(_status_code));
  std::string body =
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head><title>Error " +
      std::to_string(static_cast<int>(_status_code)) +
      "</title></head>\n"
      "<body style='text-align:center; font-family:Arial; "
      "background-color:black; color:white;'>\n"
      "<h1>Oooops! </h1>\n"
      "<p><a href='/' style='color:white;'>Go home!</a></p>\n"
      "<p>(reason: " +
      _body + ")</p>\n" + "<img src='" + cat_url + "' alt='HTTP Cat " +
      std::to_string(static_cast<int>(_status_code)) +
      "' style='max-width:100%; height:auto; margin:20px;'>\n"
      "</body></html>";

  setBody(body);
  setContentType("text/html");
}

// Converter

std::string HttpResponse::convertToString(void) {
  std::ostringstream raw_response;
  // add status line
  raw_response << "HTTP/1.1" << " " << static_cast<int>(_status_code) << " "
               << whatReasonPhrase(_status_code) << "\r\n";
  // add headers
  raw_response << "Server: Webserv" << "\r\n"
               << "Date: " << whatDateGMT() << "\r\n"
               << "Content-Length: "
               << (_body.empty() ? "0" : std::to_string(_body.length()))
               << "\r\n";
  if (!_body.empty()) {
    raw_response << "Content-Type: "
                 << (_content_type.empty() ? "text/plain" : _content_type)
                 << "\r\n";
  }
  for (auto it : _headers) {
    raw_response << capitalizeHeaderFieldName(it.first) << ": " << it.second
                 << "\r\n";
  }
  // add body
  raw_response << "\r\n" << _body;
  return raw_response.str();
}

std::string HttpResponse::whatDateGMT(void) {
  std::time_t now = std::time(nullptr);
  std::tm gmt{};
  gmtime_r(&now, &gmt);
  char buf[100];
  std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
  return std::string(buf);
}

// Helpers

std::string HttpResponse::whatReasonPhrase(
    const HttpUtils::HttpStatusCode& code) const {
  switch (code) {
    case HttpUtils::HttpStatusCode::CONTINUE:
      return "Continue";
    case HttpUtils::HttpStatusCode::SWITCHING_PROTOCOLS:
      return "Switching Protocols";
    case HttpUtils::HttpStatusCode::OK:
      return "OK";
    case HttpUtils::HttpStatusCode::CREATED:
      return "Created";
    case HttpUtils::HttpStatusCode::ACCEPTED:
      return "Accepted";
    case HttpUtils::HttpStatusCode::NON_AUTHORITATIVE_INFO:
      return "Non-Authoritative Information";
    case HttpUtils::HttpStatusCode::NO_CONTENT:
      return "No Content";
    case HttpUtils::HttpStatusCode::RESET_CONTENT:
      return "Reset Content";
    case HttpUtils::HttpStatusCode::PARTIAL_CONTENT:
      return "Partial Content";
    case HttpUtils::HttpStatusCode::MULTIPLE_CHOICES:
      return "Multiple Choices";
    case HttpUtils::HttpStatusCode::MOVED_PERMANENTLY:
      return "Moved Permanently";
    case HttpUtils::HttpStatusCode::FOUND:
      return "Found";
    case HttpUtils::HttpStatusCode::SEE_OTHER:
      return "See Other";
    case HttpUtils::HttpStatusCode::NOT_MODIFIED:
      return "Not Modified";
    case HttpUtils::HttpStatusCode::USE_PROXY:
      return "Use Proxy";
    case HttpUtils::HttpStatusCode::TEMPORARY_REDIRECT:
      return "Temporary Redirect";
    case HttpUtils::HttpStatusCode::BAD_REQUEST:
      return "Bad Request";
    case HttpUtils::HttpStatusCode::UNAUTHORIZED:
      return "Unauthorized";
    case HttpUtils::HttpStatusCode::FORBIDDEN:
      return "Forbidden";
    case HttpUtils::HttpStatusCode::NOT_FOUND:
      return "Not Found";
    case HttpUtils::HttpStatusCode::METHOD_NOT_ALLOWED:
      return "Method Not Allowed";
    case HttpUtils::HttpStatusCode::NOT_ACCEPTABLE:
      return "Not Acceptable";
    case HttpUtils::HttpStatusCode::PROXY_AUTHENTICATION_REQUIRED:
      return "Proxy Authentication Required";
    case HttpUtils::HttpStatusCode::REQUEST_TIMEOUT:
      return "Request Timeout";
    case HttpUtils::HttpStatusCode::CONFLICT:
      return "Conflict";
    case HttpUtils::HttpStatusCode::GONE:
      return "Gone";
    case HttpUtils::HttpStatusCode::LENGTH_REQUIRED:
      return "Length Required";
    case HttpUtils::HttpStatusCode::PAYLOAD_TOO_LARGE:
      return "Content Too Large";
    case HttpUtils::HttpStatusCode::URI_TOO_LONG:
      return "URI Too Long";
    case HttpUtils::HttpStatusCode::UNSUPPORTED_MEDIA_TYPE:
      return "Unsupported Media Type";
    case HttpUtils::HttpStatusCode::RANGE_NOT_SATISFIABLE:
      return "Range Not Satisfiable";
    case HttpUtils::HttpStatusCode::EXPECTATION_FAILED:
      return "Expectation Failed";
    case HttpUtils::HttpStatusCode::I_AM_TEAPOD:
      return "I'm a teapot";
    case HttpUtils::HttpStatusCode::TOO_MANY_REQUESTS:
      return "Too Many Requests";
    case HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR:
      return "Internal Server Error";
    case HttpUtils::HttpStatusCode::NOT_IMPLEMENTED:
      return "Not Implemented";
    case HttpUtils::HttpStatusCode::BAD_GATEWAY:
      return "Bad Gateway";
    case HttpUtils::HttpStatusCode::SERVICE_UNAVAILABLE:
      return "Service Unavailable";
    case HttpUtils::HttpStatusCode::GATEWAY_TIMEOUT:
      return "Gateway Timeout";
    case HttpUtils::HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED:
      return "HTTP Version Not Supported";
    default:
      return "Unknown";
  }
}

std::string HttpResponse::capitalizeHeaderFieldName(
    const std::string& field_name) {
  std::string name = field_name;
  bool is_uppercase = true;
  for (auto it = name.begin(); it != name.end(); ++it) {
    if (is_uppercase && std::isalpha(*it)) {
      *it = std::toupper(*it);
      is_uppercase = false;
    } else if (*it == '-') {
      is_uppercase = true;
    }
  }
  return name;
}

bool HttpResponse::isError(void) const { return _is_error_response; }

bool HttpResponse::isKeepAliveConnection(void) const {
  return _is_keep_alive_connection;
}

const std::string& HttpResponse::getBody(void) const { return _body; }

HttpUtils::HttpStatusCode HttpResponse::getStatusCode(void) const {
  return _status_code;
}

std::string HttpResponse::getStatusLine(void) const {
  std::stringstream status_line;
  status_line << "HTTP/1.1 " << static_cast<int>(_status_code) << " "
              << whatReasonPhrase(_status_code);
  return status_line.str();
}
