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

#include "HttpManager.hpp"
#include "Logger.hpp"
#include "config.hpp"

/// @brief Maximum number of pending connections in listen queue
#define WEBSERV_MAX_PENDING_CONNECTIONS 20
/// @brief Maximum number of events to process in single epoll_wait call
#define WEBSERV_MAX_EVENTS 10
/// @brief Timeout for epoll_wait in seconds
#define WEBSERV_TIMEOUT 10
/// @brief Size of buffer for reading client data (8 KB)
#define WEBSERV_BUFFER_SIZE 8192

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: ./webserv [path_to_config_file]" << std::endl;
    return 1;
  }

  ConfigParser::Config config;
  std::map<int, std::string> client_buffers;

  try {
    Logger::init("logs/webserv.log");
    config = ConfigParser::parse(argv[1]);
    Logger::info("Configuration parsed successfully from: " +
                 config.config_path);
    std::cout << config << std::endl;
  } catch (const std::exception &e) {
    Logger::error(e.what());
    std::cerr << e.what() << std::endl;
    Logger::shutdown();
    return 2;
  }

  /// assume we have only one server
  ConfigParser::ServerConfig serv_config = config.servers[0];
  HttpManager http_manager(serv_config);
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
        epoll_wait(epoll_fd, events, WEBSERV_MAX_EVENTS, WEBSERV_TIMEOUT);
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
        client_buffers[events[n].data.fd] += std::string(buffer, bytes_read);
        if (HttpUtils::isRawRequestComplete(
                client_buffers[events[n].data.fd])) {
          std::string raw_request = client_buffers[events[n].data.fd];
          client_buffers.erase(events[n].data.fd);
          Logger::info("Received " + std::to_string(bytes_read) +
                       " bytes from fd " + std::to_string(events[n].data.fd));
          /** @todo HTTP Manager
           * 1. Parse raw_request
           * 2. Handle the request
           * 3. Handle response
           *
           * start @test >>>
           */
          std::cout << "\n -----> Received from client: -----> \n"
                    << raw_request << std::endl;
          HttpRequest request;
          HttpResponse response;
          std::string response_msg =
              http_manager.processHttpRequest(raw_request, request, response);
          bool keep_connection_alive =
              http_manager.keepConnectionAlive(response);
          send(events[n].data.fd, response_msg.c_str(), response_msg.length(),
               0);
          std::cout << " -----> Sent response to client: -----> \n"
                    << events[n].data.fd << ": " << response_msg << std::endl;
          if (keep_connection_alive == false) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
            close(events[n].data.fd);
          }
        } else {
          Logger::info("Received partial request from fd " +
                       std::to_string(events[n].data.fd) +
                       ", waiting for more data");
        }
        /** <<< end @test */
      }
    }
  }
}
