/**
 * @file Connection.hpp
 * @brief
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-24
 * @version 1.0
 */

#ifndef _CONNECTION_HPP
#define _CONNECTION_HPP

#include <unistd.h>

#include <chrono>
#include <sstream>
#include <string>

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "Webserver.hpp"

class HttpMethodHandler;
class HttpRequest;
class HttpResponse;
class Webserv;

class Connection {
 public:
  Connection() = delete;
  ~Connection();
  Connection& operator=(const Connection& other) = delete;
  Connection(const Connection& other) = delete;
  Connection(int client_socket_fd, int server_socket_fd, Webserv& webserv,
             HttpMethodHandler& method_handler);

  void processRequest(std::string&& data);
  void updateLastActiveTime(void);
  bool isTimedOut(std::chrono::seconds timeout) const;
  bool keepAlive() const;

 private:
  int _client_fd;
  int _server_fd;
  Webserv& _webserv;
  HttpMethodHandler& _method_handler;
  HttpRequest _request;
  std::string _write_buffer;
  bool _keep_alive;
  std::chrono::steady_clock::time_point _last_active;

 private:
  void buildParserErrorResponse(void);
  void buildMethodHandlerErrorResponse(HttpResponse& response);
  void sendResponse(void);
  void cleanup(void);
};

#endif  // _CONNECTION_HPP
