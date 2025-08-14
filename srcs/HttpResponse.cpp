#include "HttpResponse.hpp"

HttpResponse::HttpResponse(const HttpUtils::HttpStatusCode& code,
                           const std::string& message)
    : _http_version("HTTP/1.1"),
      _status_code(code),
      _reason_phrase(whatReasonPhrase(code)),
      _headers(),
      _body(message) {
  // handle error codes
  if (static_cast<int>(code) >= 400) {
    setBody(R"({"error": ")" +
            (message.empty() ? R"("uknown error")" : message) + R"("})");
  }

  if (code == HttpUtils::HttpStatusCode::UKNOWN) {
    setStatusCode(HttpUtils::HttpStatusCode::I_AM_TEAPOD);
    _reason_phrase = whatReasonPhrase(_status_code);
  }

  if (!_body.empty()) {
    size_t content_length = _body.length();
    insertHeader("Content-Length", std::to_string(content_length));
  }
}

HttpResponse::~HttpResponse() { _headers.clear(); }

// Setters

void HttpResponse::setHttpVersion(const std::string& version) {
  _http_version = version;
}

void HttpResponse::setStatusCode(const HttpUtils::HttpStatusCode& code) {
  if (code == HttpUtils::HttpStatusCode::UKNOWN) {
    setStatusCode(HttpUtils::HttpStatusCode::I_AM_TEAPOD);
    setBody(R"({"error": "uknown error"})");
  } else {
    _status_code = code;
  }
  _reason_phrase = whatReasonPhrase(_status_code);
}

void HttpResponse::insertHeader(const std::string& field_name,
                                const std::string& value) {
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
  auto it = _headers.find(lowercase_name);
  if (it != _headers.end()) {
    it->second += "," + value;
  } else {
    _headers.insert({lowercase_name, value});
  }
}

void HttpResponse::setBody(const std::string& body) {
  _body = body;
  size_t content_length = _body.length();
  removeHeader("Content-Length");
  insertHeader("Content-Length", std::to_string(content_length));
}

void HttpResponse::setErrorResponse(const HttpUtils::HttpStatusCode& code,
                                    const std::string& msg) {
  setStatusCode(code);
  setBody(msg);
  _reason_phrase = whatReasonPhrase(code);
}

// Getters

const std::string& HttpResponse::getHttpVersion(void) const {
  return _http_version;
}

const HttpUtils::HttpStatusCode& HttpResponse::getStatusCode(void) const {
  return _status_code;
}

const std::string& HttpResponse::getReasonPhrase(void) const {
  return _reason_phrase;
}

const std::string& HttpResponse::getHeader(
    const std::string& field_name) const {
  static const std::string empty_string = "";
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
  auto it = _headers.find(lowercase_name);
  if (it != _headers.end()) {
    return it->second;
  }
  return empty_string;
}

const std::string& HttpResponse::getBody(void) const { return _body; }

// Checkers

bool HttpResponse::hasHeader(const std::string& field_name) const {
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
  return _headers.find(lowercase_name) != _headers.end();
}

// Converter

std::string HttpResponse::convertToString(void) {
  std::ostringstream raw_response;

  // add status line
  raw_response << _http_version << " " << static_cast<int>(_status_code) << " "
               << _reason_phrase << "\r\n";
  // add headers
  for (auto it : _headers) {
    raw_response << capitalizeHeaderFieldName(it.first) << ": " << it.second
                 << "\r\n";
  }
  // add body
  raw_response << "\r\n" << _body;
  return raw_response.str();
}

// Helpers

std::string HttpResponse::whatReasonPhrase(
    const HttpUtils::HttpStatusCode& code) {
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

void HttpResponse::removeHeader(const std::string& field_name) {
  std::string lowercase_name = HttpUtils::toLowerCase(field_name);
  _headers.erase(lowercase_name);
}
