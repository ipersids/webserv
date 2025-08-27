#include "Webserv.hpp"

// Constructor and destructor

Webserv::Webserv(const std::string &config_path) {
  (void)config_path;
  /// 1. init static Logger
  /// 2. parse webserv config file from `config_path`
  /// 3. create socket for each unique port
  /// 4. give the socket FD the local address
  /// 5. listen
  /// 6. create epoll and add server sockets file descriptors to it
}

Webserv::~Webserv() {
  /// 1. iterate _port_to_servfd:
  ///    - delete server socket from epoll
  ///    - close file destrictor
  /// 2. close epoll file descriptor
  /// 3. clear _connections map (it will call destructor of Connection objects,
  /// because it is unique pointers)
}

// public methods

void Webserv::run(void) {
  /// 1. start main event loop -> while (true)
  /// 2. epoll_wait return events
  /// 3. iterate events
  ///    - if it is new connection -> addConnection(...)
  ///    - else -> handleConnection(...) and handleKeepAliveConnection(...)
  /// 4. check timouts for connections -> cleanupTimeOutConnections(...)
}

// getters

int Webserv::getPortByServerSocket(int server_socket_fd) {
  (void)server_socket_fd;
  return -1;
}

const ConfigParser::ServerConfig &Webserv::getServerConfigs(
    int server_socket_fd, const std::string &host) {
  (void)server_socket_fd;
  (void)host;
  /// 1. find config vector from _servfd_to_config using server_socket_fd
  ///    - if there is no one (what is almost impossible) -> trow
  /// 2. parse name from name:host (ex. localhost:8002)
  /// 3. iterate vector with servers configs
  ///    - if name in server_names -> return server config
  ///    - if there is no match by name, try to match exactly host IP
  ///      - if it is exactli match -> store it with flag, we need only first
  ///      match
  /// 4. If there is no match by name, check flag if there exactly match with
  /// host
  /// 5. return first config from config
  return *(*_servfd_to_config[0].begin());  /// placeholder
}

// private helper methods

void Webserv::openServerSockets(void) {
  /// 1. iterate _config
  ///    - if it is new unique port:
  ///      -  create socket for each unique port
  ///      - store info to _port_to_servfd and _servfd_to_config
  ///    - else -> add config to the vector _servfd_to_config
}

void Webserv::bindServerSockets(void) {
  /// 1. iterate _port_to_servfd
  /// 2. setServerSocketOptions(...) and bind(...)
}

void Webserv::listenServerSockets(void) {
  /// 1. iterate _port_to_servfd
  /// 2. listen(...)
}

void Webserv::createEpoll(void) {
  /// _epoll_fd
}

void Webserv::addServerSocketsToEpoll(void) {
  /// 1. iterate _port_to_servfd
  /// 2. add in epoll_ctl(...)
}

void Webserv::addConnection(int server_socket_fd) {
  (void)server_socket_fd;
  /// 1. create client socket fd
  /// 2. accept connection
  /// 3. add to epoll
  /// 4. create connection and add it as unique poiner to _connections
}

void Webserv::handleConnection(int client_socket_fd) {
  (void)client_socket_fd;
  /// 1. read data from event
  ///    - if bytes_read == -1 -> return;
  ///    - disconnect client if received data is empty;
  ///    - else -> call
  ///    _connections[client_socket_fd]->processRequest(...) (method in
  ///    Connection class)
}

void Webserv::cleanupTimeOutConnections(void) {
  /// 1. iterate _connections
  /// 2. call Connection method isTimedOut(...)
  ///    - if tru -> erase connection from _connections
  ///    - else -> continue
}

void Webserv::handleKeepAliveConnection(int client_socket_fd) {
  (void)client_socket_fd;
  /// 1. iterate _connections
  /// 2. call Connection method keepAlive(...)
  ///    - if tru -> erase connection from _connections
  ///    - else -> continue
}

void Webserv::setServerSocketOptions(int server_socket_fd) {
  (void)server_socket_fd;
  /// setsockopt(...), useful flags:
  /// SO_REUSEADDR, SO_REUSEPORT
}

void Webserv::setClientSocketOptions(int client_socket_fd) {
  (void)client_socket_fd;
  /// setsockopt(...), useful flags:
  /// SO_KEEPALIVE, SO_RCVBUF, SO_SNDBUF, SO_RCVTIMEO, SO_RCVTIMEO
}
