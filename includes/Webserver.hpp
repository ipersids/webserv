#ifndef _WEBSERV_HPP
#define _WEBSERV_HPP

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Connection.hpp"
#include "HttpMethodHandler.hpp"
#include "Logger.hpp"
#include "Config.hpp"

extern volatile std::sig_atomic_t shutdown_requested;

class Connection;

/// @brief Maximum number of pending connections in listen queue
#define WEBSERV_MAX_PENDING_CONNECTIONS 20
/// @brief Maximum number of events to process in single epoll_wait call
#define WEBSERV_MAX_EVENTS 1024
/// @brief Timeout for epoll_wait in milliseconds
#define WEBSERV_TIMEOUT_MS 30000
/// @brief Timeout for connections in seconds
#define WEBSERV_CONNECTION_TIMEOUT_SEC 65
/// @brief Application buffer: for reading client data (16 KB)
#define WEBSERV_BUFFER_SIZE 16384

#define DEFAULT_CONFIG_PATH "tests/test-configs/test.conf"

#define DEFAULT_LOG_PATH "logs/webserv.log"

/// @brief If wait time is zero, it is non-blocking
int constexpr NONBLOCKING = 0;

class Webserv {
 public:
  Webserv() = delete;
  Webserv(const std::string &config_path);
  ~Webserv();
  Webserv &operator=(const Webserv &other) = delete;
  Webserv(const Webserv &other) = delete;
  static void set_exit_to_true(int useless);

  void run(void);

  int getPortByServerSocket(int server_socket_fd);
  const ConfigParser::ServerConfig &getServerConfigs(int server_socket_fd,
                                                     const std::string &host);

 private:
  ConfigParser::Config _config;
  std::unordered_map<int, int> _port_to_servfd;
  std::unordered_map<int, std::vector<ConfigParser::ServerConfig *>>
      _servfd_to_config;
  int _epoll_fd;
  std::unordered_map<int, std::unique_ptr<Connection>> _connections;
  char _buffer[WEBSERV_BUFFER_SIZE];
  HttpMethodHandler _method_handler;

 private:
  // helper functions
  void openServerSockets(void);
  void bindServerSockets(void);
  void listenServerSockets(void);
  void createEpoll(void);
  void addServerSocketsToEpoll(void);
  void addConnection(int server_socket_fd);
  void handleConnection(int client_socket_fd);
  void cleanupTimeOutConnections(void);
  void handleKeepAliveConnection(int client_socket_fd);
  void setServerSocketOptions(int server_socket_fd);
  void setClientSocketOptions(int client_socket_fd);
};

#endif  // _WEBSERV_HPP
