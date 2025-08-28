#include "Webserver.hpp"
#include "defaults.hpp"

// Constructor and destructor

Webserv::Webserv(const std::string &config_path) {

	/// 1. init static Logger

	Logger::init(DEFAULT_LOG_PATH);
	/// 2. parse webserv config file from `config_path`

	_config = ConfigParser::parse(config_path);

	/// 3. create socket for each unique port

	openServerSockets();


	/// 4. give the socket FD the local address

	bindServerSockets(); //maybe this?


	/// 5. listen

	listenServerSockets();


	/// 6. create epoll and add server sockets file descriptors to it

	createEpoll();
	addServerSocketsToEpoll();

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
	///    - if there is no one (what is almost impossible) -> throw
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

	for (auto it = _config.servers.begin(); it != _config.servers.end(); it++)
	{
		
		if (_port_to_servfd.find(it->port) == nullptr)
		{
			int server_socketfd = socket(AF_INET, SOCK_STREAM, 0);
			if (server_socketfd == -1)
			{
				throw std::runtime_error("Socket creation failed");
				Logger::error("The socket() system call failed.");
				Logger::shutdown();
			}
			_port_to_servfd.emplace(it->port, server_socketfd);
			_servfd_to_config.emplace(it->port, std::vector<ConfigParser::ServerConfig*>{ &*it }
);

		}
		else 
		{
			_servfd_to_config[it->port].push_back(&*it);
		}
	}
}

void Webserv::bindServerSockets(void) {
	/// 1. iterate _port_to_servfd
	/// 2. setServerSocketOptions(...) and bind(...)

	  struct sockaddr_in serv_addr, cli_addr;

for(auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
{
	int enable = 1;
	if (setsockopt(it->second, SOL_SOCKET, SO_REUSEADDR, &enable,
								sizeof(enable)) == -1) {
		Logger::error("Setting the socket options with setsockopt() failed.");
		Logger::shutdown();
		throw std::runtime_error("Setting socket options failed");
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(it->first);
	if (bind(it->second, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
		Logger::error("The bind() method failed.");
		Logger::shutdown();
		throw std::runtime_error("Binding socket failed");
	}
}




}

void Webserv::listenServerSockets(void) {
	for(auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
	{
		 if (listen(it->second, WEBSERV_MAX_PENDING_CONNECTIONS) == -1) {
    Logger::error("The listen() function failed.");
    Logger::shutdown();
    throw std::runtime_error("Listen failed");
  }
	}
}

void Webserv::createEpoll(void) {
	_epoll_fd = epoll_create1(0);
  if (_epoll_fd == -1) {
    Logger::error("The epoll_create1() function failed.");
    Logger::shutdown();
     throw std::runtime_error("Epoll creation failed");
  }
}

void Webserv::addServerSocketsToEpoll(void) {
	/// 1. iterate _port_to_servfd
	/// 2. add in epoll_ctl(...)

	for(auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
	{
		struct epoll_event ev, events[WEBSERV_MAX_EVENTS];
  	ev.events = EPOLLIN;
  	ev.data.fd = it->second;
  	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, it->second, &ev) == -1) {
    Logger::error("The epoll_ctl() failed: server listen socket.");
    Logger::shutdown();
    throw std::runtime_error("Epoll_ctl() failed");
  	}
	}
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
