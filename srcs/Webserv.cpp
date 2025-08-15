/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/15 15:31:58 by jrimpila          #+#    #+#             */
/*   Updated: 2025/08/15 17:17:39 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

#define WEBSERV_MAX_PENDING_CONNECTIONS 3

int Webserv::createConfig(char **argv)
{

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
    serv_config = config.servers[0];
    HttpManager setHttpManager(serv_config);
    http_manager = &setHttpManager;
   

  return 0;
}


int Webserv::createSocket()
{
    server_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socketfd == -1) {
      Logger::error("The socket() system call failed.");
      Logger::shutdown();
      return 3;
    }

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

   if (listen(server_socketfd, maxPendingConnections) == -1) {
    Logger::error("The listen() function failed.");
    Logger::shutdown();
    return 6;
  }

  return 0;
}




int Webserv::createEpoll()
{
    epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    Logger::error("The epoll_create1() function failed.");
    Logger::shutdown();
    return 7;
  }

  
  ev.events = EPOLLIN;
  ev.data.fd = server_socketfd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socketfd, &ev) == -1) {
    Logger::error("The epoll_ctl() failed: server listen socket.");
    Logger::shutdown();
    return 8;
  }

  amount_fds = -1;
  client_socketfd = -1;
  client_socklen = sizeof(cli_addr);

    return 0;
}


int  Webserv::run()
{
   
  for (;;) {
    amount_fds =
        epoll_wait(epoll_fd, events, maxEvents, timeout);
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
        char buffer[bufferSize];
        ssize_t bytes_read =
            recv(events[n].data.fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
          if (bytes_read == 0) {
            Logger::info("Client disconnected on fd " +
                         std::to_string(events[n].data.fd));
          } else {
            Logger::error("Error reading from client fd " +
                          std::to_string(events[n].data.fd) + ": " +
                          std::string(strerror(errno)));
          }
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
          continue;
        }
        buffer[bytes_read] = '\0';
        std::string raw_request(buffer, bytes_read);
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
            http_manager -> processHttpRequest(raw_request, request, response);
        bool keep_connection_alive = http_manager ->keepConnectionAlive(response);
        send(events[n].data.fd, response_msg.c_str(), response_msg.length(), 0);
        std::cout << " -----> Sent response to client: -----> \n"
                  << events[n].data.fd << ": " << response_msg << std::endl;
        if (keep_connection_alive == false) {
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
        }
        /** <<< end @test */
      }
    }
    }
    return 0;
}