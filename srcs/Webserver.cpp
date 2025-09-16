#include "Webserver.hpp"

#include "Config.hpp"

// Constructor and destructor

Webserv::Webserv(const std::string &config_path) {
  // init static Logger
  try {
    Logger::init("logs/webserv.log");
    std::atexit(Logger::shutdown);
  } catch (const std::filesystem::filesystem_error &e) {
    std::string msg = e.what();
    Logger::error("Logger initialisation failed" + msg);
    throw std::runtime_error("Logger initialisation failed" + msg);
  } catch (const std::exception &e) {
    std::string msg = e.what();
    Logger::error("Logger initialisation failed" + msg);
    throw std::runtime_error("Logger initialisation failed" + msg);
  } catch (...) {
    Logger::error("Logger initialisation failed with uknown error");
    throw std::runtime_error("Logger initialisation failed with uknown error");
  }
  // parse webserv config file
  try {
    _config = ConfigParser::parse(config_path);
    Logger::info("Webserv config file parsed successfully from: " +
                 _config.config_path);
    DBG("[PARSED CONFIG] >>>>>>\n" << _config);
  } catch (const std::exception &e) {
    std::string msg = e.what();
    Logger::error("Webserv config file parsing failed: " + msg);
    throw std::runtime_error("Webserv config file parsing failed: " + msg);
  } catch (...) {
    std::string msg = "Webserv config file parsing failed with uknown error";
    Logger::error(msg);
    throw std::runtime_error(msg);
  }
  /// 3. create socket for each unique port
  openServerSockets();
  /// 4. give the socket FD the local address
  bindServerSockets();
  /// 5. listen
  listenServerSockets();
  /// 6. create epoll and add server sockets file descriptors to it
  createEpoll();
  addServerSocketsToEpoll();
}

Webserv::~Webserv() {
  for (auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++) {
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, it->second, nullptr) == -1) {
      Logger::warning("Failed to remove server fd from epoll " +
                      std::to_string(it->second) + ": " + strerror(errno));
    }
    if (close(it->second) == -1) {
      Logger::warning("Failed to close server fd " +
                      std::to_string(it->second) + ": " + strerror(errno));
    }
  }
  close(_epoll_fd);
  _connections.clear();
}

// public methods

void Webserv::run(void) {
  struct epoll_event events[WEBSERV_MAX_EVENTS];
  while (shutdown_requested == false) {
    int events_total =
        epoll_wait(_epoll_fd, events, WEBSERV_MAX_EVENTS, NONBLOCKING);
    if (events_total == -1) {
      Logger::error("The epoll_wait() system call failed: " +
                    std::string(strerror(errno)));
      throw std::runtime_error(std::string(strerror(errno)));
    }
    for (int index = 0; index < events_total; index++) {
      int fd = events[index].data.fd;
      auto it = _connections.find(fd);
      if (it == _connections.end()) {
        addConnection(fd);  // If it is not in connections it is a server fd
      } else {
        handleConnection(fd);  // if it was found it is a client fd
        handleKeepAliveConnection(fd);
      }
    }
    cleanupTimeOutConnections();
  }
}

int Webserv::getPortByServerSocket(int server_socket_fd) {
  for (auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++) {
    if (it->second == server_socket_fd) return it->first;
  }
  return -1;
}

const ConfigParser::ServerConfig &Webserv::getServerConfigs(
    int server_socket_fd, const std::string &host) {
  auto it = _servfd_to_config.find(server_socket_fd);

  if (it == _servfd_to_config.end() || it->second.empty()) {
    throw std::runtime_error("No server configurations for socket fd " +
                             std::to_string(server_socket_fd));
  }

  const std::vector<ConfigParser::ServerConfig *> &servers = it->second;

  std::string _host = (host.find_last_of(":") == std::string::npos)
                          ? host
                          : host.substr(0, host.find_last_of(":"));

  const ConfigParser::ServerConfig *_serv = nullptr;
  bool is_match_by_serv_host = false;
  for (const ConfigParser::ServerConfig *serv : servers) {
    if (std::find(serv->server_names.begin(), serv->server_names.end(),
                  _host) != serv->server_names.end()) {
      return *serv;
    }
    if (!is_match_by_serv_host && serv->host == _host) {
      _serv = serv;
      is_match_by_serv_host = true;
    }
  }

  if (is_match_by_serv_host && _serv != nullptr) {
    return *_serv;
  }

  return *(servers[0]);
}

// private helper methods

void Webserv::openServerSockets(void) {
  for (ConfigParser::ServerConfig &serv : _config.servers) {
    if (_port_to_servfd.find(serv.port) == _port_to_servfd.end()) {
      ///			-	create socket for each unique port
      int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (server_socket_fd == -1) {
        Logger::error("The socket() system call failed: " +
                      std::string(strerror(errno)));
        throw std::runtime_error(std::string(strerror(errno)));
      }
      _port_to_servfd[serv.port] = server_socket_fd;
      _servfd_to_config[server_socket_fd].push_back(&serv);
    } else {
      // two servers can listen on the same port
      int tmp_socket = _port_to_servfd[serv.port];
      _servfd_to_config[tmp_socket].push_back(&serv);
    }
  }
}

void Webserv::bindServerSockets(void) {
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  for (auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++) {
    serv_addr.sin_port = htons(it->first);
    setServerSocketOptions(it->second);
    if (bind(it->second, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
      Logger::error("The bind() method failed.");
      throw std::runtime_error("Binding socket failed");
    }
  }
}

void Webserv::listenServerSockets(void) {
  for (auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++) {
    if (listen(it->second, WEBSERV_MAX_PENDING_CONNECTIONS) == -1) {
      Logger::error("The listen() function failed.");
      throw std::runtime_error("Listen failed");
    }
  }
}

void Webserv::createEpoll(void) {
  _epoll_fd = epoll_create1(0);
  if (_epoll_fd == -1) {
    Logger::error("The epoll_create1() function failed.");
    throw std::runtime_error("Epoll creation failed");
  }
}

void Webserv::addServerSocketsToEpoll(void) {
  for (auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = it->second;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, it->second, &ev) == -1) {
      Logger::error("The epoll_ctl() failed: server listen socket.");
      throw std::runtime_error("Epoll_ctl() failed");
    }
  }
}

void Webserv::addConnection(int server_socket_fd) {
  int client_socketfd = -1;
  struct sockaddr_in cli_addr;
  size_t client_socklen = sizeof(cli_addr);

  client_socketfd = accept(server_socket_fd, (struct sockaddr *)&cli_addr,
                           (socklen_t *)&client_socklen);
  if (client_socketfd == -1) {
    Logger::error("Failed to accept connection: " +
                  std::string(strerror(errno)));
    throw std::runtime_error("Creating client socket failed");
  }
  Logger::info("New connection accepted on fd " +
               std::to_string(client_socketfd));
  /// 3. add to epoll
  setClientSocketOptions(client_socketfd);
  struct epoll_event client_ev;
  client_ev.events = EPOLLIN;
  client_ev.data.fd = client_socketfd;
  if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_socketfd, &client_ev) == -1) {
    Logger::error("Failed to add client to epoll");
    close(client_socketfd);
    throw std::runtime_error("Adding to epoll failed");
  }

  /// 4. create connection and add it as unique poiner to _connections
  _connections[client_socketfd] = std::make_unique<Connection>(
      client_socketfd, server_socket_fd, *this, _method_handler);
  Logger::info("New connection (fd " + std::to_string(client_socketfd) +
               ") accepted on port " +
               std::to_string(getPortByServerSocket(server_socket_fd)));
}

void Webserv::handleConnection(int client_socket_fd) {
  ssize_t bytes_read = recv(client_socket_fd, _buffer, sizeof(_buffer), 0);
  if (bytes_read <= 0) {
    Logger::warning("Client disconnected on fd " +
                    std::to_string(client_socket_fd));
    ///    - disconnect client if received data is empty;
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_socket_fd, nullptr);
    _connections.erase(client_socket_fd);
  } else {
    _buffer[bytes_read] = '\0';
    DBG("----------- RECEIVED REQUEST -----------\n" << _buffer);
    _connections[client_socket_fd]->processRequest(
        std::string(_buffer, bytes_read));
  }
}

void Webserv::cleanupTimeOutConnections(void) {

  auto it = _connections.begin();
  while (it != _connections.end()) {
    if (it->second->isTimedOut(
            std::chrono::seconds(WEBSERV_CONNECTION_TIMEOUT_SEC)) ==
        true)
    {
      Logger::info("Connection timed out: fd=" + std::to_string(it->first));
      if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, it->first, nullptr) == -1) {
        Logger::warning("Failed to remove client fd from epoll " +
                        std::to_string(it->first) + ": " + strerror(errno));
      }
      it = _connections.erase(it);
    } else
      it++;
  }
}

void Webserv::handleKeepAliveConnection(int client_socket_fd) {
  std::unordered_map<int, std::unique_ptr<Connection>>::iterator it =
      _connections.find(client_socket_fd);

  if (it != _connections.end()) {
    if (_connections[client_socket_fd]->keepAlive()) {
      return;
    }
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_socket_fd, nullptr) == -1) {
      Logger::warning("Failed to remove client fd from epoll " +
                      std::to_string(client_socket_fd) + ": " +
                      strerror(errno));
    }
    it = _connections.erase(it);
  }
}

void Webserv::setServerSocketOptions(int server_socket_fd) {
  const int enable = 1;
  /**
   * @note SO_REUSEADDR
   * allow immediate restart (ex.: after ^C we can run ./webserv
   * again and bind() will not fail because of timeout)
   */
  if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable,
                 sizeof(enable)) == -1) {
    Logger::error("The setsockopt() system call failed: SO_REUSEADDR " +
                  std::string(strerror(errno)));
    throw std::runtime_error(std::string(strerror(errno)));
  }
  /**
   * @note SO_REUSEPORT
   * enables load balancing across multiple processes (allows
   * multiple sockets to bind to the same port if using fork/multiple processes)
   */
  if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable,
                 sizeof(enable)) == -1) {
    Logger::error("The setsockopt() system call failed: SO_REUSEPORT " +
                  std::string(strerror(errno)));
    throw std::runtime_error(std::string(strerror(errno)));
  }
}

void Webserv::setClientSocketOptions(int client_socket_fd) {
  int enable = 1;

  if (setsockopt(client_socket_fd, SOL_SOCKET, SO_KEEPALIVE, &enable,
                 sizeof(enable)) == -1) {
    Logger::error("Setting the socket options with setsockopt() failed.");
    throw std::runtime_error("Setting socket options failed");
  }

  int buffer_size = WEBSERV_BUFFER_SIZE;

  if (setsockopt(client_socket_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size,
                 sizeof(buffer_size)) == -1) {
    Logger::error("Setting the socket options with setsockopt() failed.");
    throw std::runtime_error("Setting socket options failed");
  }
  if (setsockopt(client_socket_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size,
                 sizeof(buffer_size)) == -1) {
    Logger::error("Setting the socket options with setsockopt() failed.");
    throw std::runtime_error("Setting socket options failed");
  }

  struct timeval timeout;
  timeout.tv_sec = WEBSERV_TIMEOUT_MS / 1000;
  timeout.tv_usec = WEBSERV_TIMEOUT_MS % 1000 * 1000;

  if (setsockopt(client_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout)) == -1) {
    Logger::error("Setting the socket options with setsockopt() failed.");
    throw std::runtime_error("Setting socket options failed");
  }
}

void Webserv::set_exit_to_true(int signal) { shutdown_requested = signal; }
