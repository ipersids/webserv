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
#include "Webserv.hpp"

/// @brief Maximum number of pending connections in listen queue
#define WEBSERV_MAX_PENDING_CONNECTIONS 3
/// @brief Maximum number of events to process in single epoll_wait call
#define WEBSERV_MAX_EVENTS 10
/// @brief Timeout for epoll_wait in seconds
#define WEBSERV_TIMEOUT 10
/// @brief Size of buffer for reading client data (8 KB)
#define WEBSERV_BUFFER_SIZE 8192




int main(int argc, char **argv) {
  
  Webserv server;
  
  if (argc != 2) {
    std::cerr << "Usage: ./webserv [path_to_config_file]" << std::endl;
    return 1;
  }


    int retval = server.createConfig(argv);
    if (retval) return retval;

    retval = server.createSocket();
    if (retval) return retval;

    retval = server.createEpoll();
    if (retval) return retval;


  server.run();
  

 



}

