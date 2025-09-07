/**
 * @file test_server_main.cpp
 * @brief Test HTTP server implementation using epoll for event-driven I/O
 * @author Julia Persidskaia (ipersids)
 * @date 2025-08-12
 * @version 1.0
 *
 * @note This is a test implementation and should not be used in production
 *
 */

#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Webserver.hpp"
#include "config.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: ./webserv [path_to_config_file]" << std::endl;
    return 1;
  }

  ConfigParser::Config config;
  std::map<int, HttpRequest> client_buffers;
  HttpMethodHandler _method_handler;

  try {
    Logger::init("logs/webserv.log");
    config = ConfigParser::parse(argv[1]);
    Logger::info("Configuration parsed successfully from: " +
                 config.config_path);
  } catch (const std::exception &e) {
    Logger::error(e.what());
    std::cerr << e.what() << std::endl;
    Logger::shutdown();
    return 2;
  }

  /// assume we have only one server
  ConfigParser::ServerConfig serv_config = config.servers[0];
  std::cout << serv_config << std::endl;
  int server_socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socketfd == -1) {
    Logger::error("The socket() system call failed.");
    Logger::shutdown();
    return 3;
  }

  struct sockaddr_in serv_addr, cli_addr;
  int enable = 1;
  if (setsockopt(server_socketfd, SOL_SOCKET, SO_REUSEADDR, &enable,
                 sizeof(enable)) == -1) {
    Logger::error("Setting the socket options with setsockopt() failed.");
    Logger::shutdown();
    return 4;
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(serv_config.port);
  if (bind(server_socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    Logger::error("The bind() method failed.");
    Logger::shutdown();
    return 5;
  }

  if (listen(server_socketfd, WEBSERV_MAX_PENDING_CONNECTIONS) == -1) {
    Logger::error("The listen() function failed.");
    Logger::shutdown();
    return 6;
  }

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    Logger::error("The epoll_create1() function failed.");
    Logger::shutdown();
    return 7;
  }

  struct epoll_event ev, events[WEBSERV_MAX_EVENTS];
  ev.events = EPOLLIN | EPOLLOUT;
  ev.data.fd = server_socketfd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socketfd, &ev) == -1) {
    Logger::error("The epoll_ctl() failed: server listen socket.");
    Logger::shutdown();
    return 8;
  }

  int amount_fds = -1;
  int client_socketfd = -1;
  size_t client_socklen = sizeof(cli_addr);
  struct epoll_event client_ev;
  for (;;) {
    amount_fds =
        epoll_wait(epoll_fd, events, WEBSERV_MAX_EVENTS, WEBSERV_TIMEOUT_MS);
    if (amount_fds == -1) {
      Logger::error("The epoll_wait() failed.");
      Logger::shutdown();
      return 9;
    }

    for (int n = 0; n < amount_fds; ++n) {
      if (events[n].data.fd == server_socketfd) {
        client_socketfd = accept(server_socketfd, (struct sockaddr *)&cli_addr,
                                 (socklen_t *)&client_socklen);
        if (client_socketfd == -1) {
          Logger::error("Failed to accept connection: " +
                        std::string(strerror(errno)));
          continue;
        }

        Logger::info("New connection accepted on fd " +
                     std::to_string(client_socketfd));

        client_ev.events = EPOLLIN | EPOLLOUT;
        client_ev.data.fd = client_socketfd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socketfd, &client_ev) ==
            -1) {
          Logger::error("Failed to add client to epoll");
          close(client_socketfd);
          continue;
        }
        client_buffers[client_socketfd];
      } else {
        char buffer[WEBSERV_BUFFER_SIZE];
        ssize_t bytes_read =
            recv(events[n].data.fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

        if (bytes_read == -1) {
          continue;
        }

        if (bytes_read == 0) {
          Logger::info("Client disconnected on fd " +
                       std::to_string(events[n].data.fd));

          client_buffers.erase(events[n].data.fd);  // Clean up buffer
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
          continue;
        }

        buffer[bytes_read] = '\0';

        HttpRequestParser::Status status = HttpRequestParser::parseRequest(
            std::move(buffer), client_buffers[events[n].data.fd]);
        std::stringstream msg;
        if (status == HttpRequestParser::Status::WAIT_FOR_DATA) {
          msg << " -> Received partial request from client fd "
              << events[n].data.fd << ", waiting for more data";
          Logger::info(msg.str());
          DBG("\n------- RECEIVED PARTIAL REQUEST [0]--------\n" << buffer);
          continue;
        } else {
          DBG("\n----------- RECEIVED REQUEST [1] -----------\n" << buffer);
        }

        HttpRequest &_request = client_buffers[events[n].data.fd];
        std::string _write_buffer;
        if (status == HttpRequestParser::Status::ERROR) {
          if (_request.hasHeader("Host")) {
            msg << ", Host: " << _request.getHeader("Host") << "\n";
          }
          msg << "\t-> Failed to parse request from client fd "
              << events[n].data.fd << ": " << _request.getErrorMessage() << " ("
              << static_cast<int>(_request.getStatusCode()) << ")";
          Logger::error(msg.str());
          HttpResponse response;
          response.setStatusCode(_request.getStatusCode());
          response.setBody(_request.getErrorMessage());
          response.setErrorPageBody(serv_config);
          response.insertHeader("Connection", "close");
          _write_buffer = std::move(response.convertToString());
          send(events[n].data.fd, _write_buffer.c_str(), _write_buffer.length(),
               0);
          client_buffers.erase(events[n].data.fd);
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
          DBG("\n----------- SENDING RESPONSE [2] -----------\n"
              << _write_buffer);
          continue;
        }

        HttpResponse response =
            _method_handler.processMethod(_request, serv_config);

        msg << ", Host: " << _request.getHeader("Host") << "\n"
            << "\t-> Received request: \t\t" << _request.getRequestLine()
            << "\n\t-> Sending response: \t\t" << response.getStatusLine();

        if (response.isError()) {
          msg << " (reason: " << response.getBody() << ")";
          Logger::error(msg.str());
          response.setErrorPageBody(serv_config);
          response.insertHeader("Connection", "close");
          _write_buffer = std::move(response.convertToString());
          DBG("\n----------- SENDING RESPONSE [3] -----------\n"
              << _write_buffer);
        } else {
          Logger::info(msg.str());
          response.setConnectionHeader(_request.getHeader("Connection"),
                                       _request.getHttpVersion());
          _write_buffer = std::move(response.convertToString());
          DBG("\n----------- SENDING RESPONSE [4] -----------\n"
              << _write_buffer);
        }
        send(events[n].data.fd, _write_buffer.c_str(), _write_buffer.length(),
             0);
        _request.reset();
        if (response.isError() || _request.getHeader("Connection") == "close") {
          client_buffers.erase(events[n].data.fd);
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
        }
      }
    }
  }
  if (shutdown_requested) return 128 + shutdown_requested;
  return 0;
}
