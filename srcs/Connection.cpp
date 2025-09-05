/**
 * @file Connection.cpp
 * @brief
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-24
 * @version 1.0
 */

#include "Connection.hpp"

// constructor and destructor

Connection::~Connection() {
  if (_client_fd >= 0) {
    if (close(_client_fd) == -1) {
      Logger::warning("Failed to close client fd " +
                      std::to_string(_client_fd) + ": " + strerror(errno));
    }
  }
}

Connection::Connection(int client_socket_fd, int server_socket_fd,
                       Webserv& webserv, HttpMethodHandler& method_handler)
    : _client_fd(client_socket_fd),
      _server_fd(server_socket_fd),
      _webserv(webserv),
      _method_handler(method_handler),
      _request(),
      _write_buffer(""),
      _keep_alive(true),
      _last_active(std::chrono::steady_clock::now()) {}

// public methods

void Connection::processRequest(std::string&& data) {
  HttpRequestParser::Status status =
      HttpRequestParser::parseRequest(std::move(data), _request);

  std::stringstream msg;
  msg << "Port: " << _webserv.getPortByServerSocket(_server_fd);
  if (status == HttpRequestParser::Status::WAIT_FOR_DATA) {
    msg << " -> Received partial request from client fd " << _client_fd
        << ", waiting for more data";
    Logger::info(msg.str());
    return;
  }

  if (status == HttpRequestParser::Status::ERROR) {
    if (_request.hasHeader("Host")) {
      msg << ", Host: " << _request.getHeader("Host") << "\n";
    }
    msg << "\t-> Failed to parse request from client fd " << _client_fd << ": "
        << _request.getErrorMessage() << " ("
        << static_cast<int>(_request.getStatusCode()) << ")";
    Logger::error(msg.str());
    buildParserErrorResponse();
    DBG("\n----------- SENDING RESPONSE [1] -----------\n" << _write_buffer);
    sendResponse();
    return;
  }

  HttpResponse response = _method_handler.processMethod(
      _request,
      _webserv.getServerConfigs(_server_fd, _request.getHeader("Host")));

  msg << ", Host: " << _request.getHeader("Host") << "\n"
      << "\t-> Received request: \t\t" << _request.getRequestLine()
      << "\n\t-> Sending response: \t\t" << response.getStatusLine();

  if (response.isError()) {
    msg << " (reason: " << response.getBody() << ")";
    Logger::error(msg.str());
    buildMethodHandlerErrorResponse(response);
    DBG("----------- SENDING RESPONSE [2] -----------\n" << _write_buffer);
  } else {
    Logger::info(msg.str());
    response.setConnectionHeader(_request.getHeader("Connection"),
                                 _request.getHttpVersion());
    _keep_alive = response.isKeepAliveConnection();
    _write_buffer = (response.convertToString());
    DBG("----------- SENDING RESPONSE [3] -----------\n" << _write_buffer);
  }
  sendResponse();
}

// public helpers

bool Connection::isTimedOut(std::chrono::seconds timeout) const {
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::seconds elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(now - _last_active);
  return elapsed >= timeout;
}

bool Connection::keepAlive() const { return _keep_alive; }

void Connection::updateLastActiveTime(void) {
  _last_active = std::chrono::steady_clock::now();
}

// private

void Connection::sendResponse(void) {
  updateLastActiveTime();
  ssize_t bytes =
      send(_client_fd, _write_buffer.c_str(), _write_buffer.length(), 0);

  if (bytes == -1) {
    Logger::error("Failed to send response to client fd " +
                  std::to_string(_client_fd) + ": " +
                  std::string(strerror(errno)));
  } else {
    Logger::info("Successfully sent response to client fd " +
                 std::to_string(_client_fd) +
                 ", bytes sent: " + std::to_string(bytes));
  }
  cleanup();
}

void Connection::cleanup(void) {
  _request.reset();
  _write_buffer.clear();
}

void Connection::buildParserErrorResponse(void) {
  HttpResponse response;
  response.setStatusCode(_request.getStatusCode());
  response.setBody(_request.getErrorMessage());
  response.setErrorPageBody(
      _webserv.getServerConfigs(_server_fd, _request.getHeader("Host")));
  response.insertHeader("Connection", "close");
  _keep_alive = false;
  _write_buffer = (response.convertToString());
}

void Connection::buildMethodHandlerErrorResponse(HttpResponse& response) {
  response.setErrorPageBody(
      _webserv.getServerConfigs(_server_fd, _request.getHeader("Host")));
  response.setConnectionHeader(_request.getHeader("Connection"),
                               _request.getHttpVersion());
  _keep_alive = response.isKeepAliveConnection();
  _write_buffer = (response.convertToString());
}
