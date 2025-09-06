#include "Webserver.hpp"
#include "config.hpp"

// Constructor and destructor

Webserv::Webserv(const std::string &config_path) {

	/// 1. init static Logger

	Logger::init(DEFAULT_LOG_PATH);
	/// 2. parse webserv config file from `config_path`
	std::atexit(Logger::shutdown);

	try {
	_config = ConfigParser::parse(config_path);
	}
	catch(...)
	{
		Logger::error("Parse failed\n");
		throw std::runtime_error("Parse failed\n");
	}

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

	for (auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
	{
		if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, it->second, nullptr) == -1) {
            Logger::error("Removing socket from epoll failed\n");
        }
		close(it->second);
	}

	close (_epoll_fd);

	_connections.clear();

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

	struct epoll_event events[WEBSERV_MAX_EVENTS];

	while(shutdown_requested == false)
	{
		int events_total =  epoll_wait(_epoll_fd, events, WEBSERV_MAX_EVENTS, NONBLOCKING);
	
		for(int index = 0; index < events_total; index++)
		{
			int fd = events[index].data.fd;
			auto it = _connections.find(fd);
			if (it == _connections.end()) {
				addConnection(fd); //If it is not in connections it is a server fd
			} else {
				handleConnection(fd); // if it was found it is a client fd
			}
		}
		cleanupTimeOutConnections();
	}



}

// getters

int Webserv::getPortByServerSocket(int server_socket_fd) {
	

	for (auto it = _port_to_servfd.begin() ; it == _port_to_servfd.end(); it++)
	{
		if (it->second == server_socket_fd)
			return it->first;
	}
	return -1;
}

const ConfigParser::ServerConfig &Webserv::getServerConfigs(
		int server_socket_fd, const std::string &host) {

	/// 1. find config vector from _servfd_to_config using server_socket_fd
	///    - if there is no one (what is almost impossible) -> throw
	
	(void) server_socket_fd;
	size_t colon_pos = host.find_last_of(':');

	if (colon_pos != std::string_view::npos) {
    	std::string myHost = host.substr(0, colon_pos); 
	
		for (auto it = _servfd_to_config.begin(); it != _servfd_to_config.end(); it++)
		{
			for (auto it2 = (it)->second.begin(); it2 != it->second.end(); it2++)
			{
				std::vector<std::string> &helper = (*it2)->server_names;
				for (auto it3 = helper.begin(); it3 != helper.end(); it3++)
				{
					if (*(it3) == myHost)
						return *(*it2);
				}
			}

		}

		size_t pos = host.find_last_of(':');

		int myPort = std::stoi(host.substr(pos + 1));

		for (auto it = _servfd_to_config.begin(); it != _servfd_to_config.end(); it++)
		{
			for (auto it2 = (it)->second.begin(); it2 != it->second.end(); it2++)
			{
				if ((*it2)->port == myPort)
					return *(*it2);
			}
		}
	}

	if (_config.servers.empty()) {
    throw std::runtime_error("No server configurations available");
	}
	return *_config.servers.begin();


	

	/// 2. parse name from name:host (ex. localhost:8002)
	/// 3. iterate vector with servers configs
	///    - if name in server_names -> return server config
	///    - if there is no match by name, try to match exactly host IP
	///      - if it is exactli match -> store it with flag, we need only first
	///      match
	/// 4. If there is no match by name, check flag if there exactly match with
	/// host
	/// 5. return first config from config

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
		for (auto it2 = _config.servers.begin(); it2 != _config.servers.end(); it2++)

		if (_port_to_servfd.find(it2->port) == nullptr)
		{
			int server_socketfd = socket(AF_INET, SOCK_STREAM, 0);
			if (server_socketfd == -1)
			{
				throw std::runtime_error("Socket creation failed");
				Logger::error("The socket() system call failed.");
			}
			_port_to_servfd.emplace(it2->port, server_socketfd);
			_servfd_to_config.emplace(it2->port, std::vector<ConfigParser::ServerConfig*>{ &*it });
		}
		else 
		{
			_servfd_to_config[it2->port].push_back(&*it);
		}
	}
}

void Webserv::bindServerSockets(void) {
	/// 1. iterate _port_to_servfd
	/// 2. setServerSocketOptions(...) and bind(...)

	  struct sockaddr_in serv_addr;

for(auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
{
	setServerSocketOptions(it->second);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(it->first);
	if (bind(it->second, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
		Logger::error("The bind() method failed.");
		throw std::runtime_error("Binding socket failed");
	}
}




}

void Webserv::listenServerSockets(void) {
	for(auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
	{
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
	/// 1. iterate _port_to_servfd
	/// 2. add in epoll_ctl(...)

	for(auto it = _port_to_servfd.begin(); it != _port_to_servfd.end(); it++)
	{
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
	(void)server_socket_fd;
	/// 1. create client socket fd

	int client_socketfd = -1;
	struct sockaddr_in cli_addr;
	size_t client_socklen = sizeof(cli_addr);

	/// 2. accept connection
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

        client_ev.events = EPOLLIN | EPOLLOUT;
        client_ev.data.fd = client_socketfd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_socketfd, &client_ev) ==
            -1) {
          Logger::error("Failed to add client to epoll");
          close(client_socketfd);
          throw std::runtime_error("Adding to epoll failed");
        }

			/// 4. create connection and add it as unique poiner to _connections
		try {
		auto newConnection = std::make_unique<Connection>(client_socketfd, server_socket_fd, *this, _method_handler);
		
		_connections[client_socketfd] = std::move(newConnection);
		}
		catch (...)
		{
			throw std::runtime_error("Creating and assigning unique ptr failed"); //I was originally planning to catch it in the main but this is required for it to compile
		}
		
		
}

void Webserv::handleConnection(int client_socket_fd) {


	ssize_t bytes_read = recv(client_socket_fd, _buffer, sizeof(_buffer), 0);

	if (bytes_read < 0)
	{
		return;
	}
	else if (bytes_read == 0)
	{
		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_socket_fd, NULL);
		_connections.erase(client_socket_fd);
	}
	else
	{
		_buffer[bytes_read] = '\0';
		_connections[client_socket_fd]->processRequest(std::string(_buffer, bytes_read));
	}

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
	auto it = _connections.begin();
	while(it != _connections.end())
	{
		if (it->second->isTimedOut(std::chrono::seconds(WEBSERV_CONNECTION_TIMEOUT_SEC)) == true) //am i using this correctly?
		{
			epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, it->second->getClientFD(), NULL);
			it = _connections.erase(it);
		}
		else 
			it++;
	}
}

//seems unnecessary
void Webserv::handleKeepAliveConnection(int client_socket_fd) {
	(void)client_socket_fd;
	/// 1. iterate _connections
	/// 2. call Connection method keepAlive(...)
	///    - if tru -> erase connection from _connections
	///    - else -> continue

	auto it = _connections.begin();
	while(it != _connections.end())
	{
		if (it->second->keepAlive() == false) //Mistake in the instructions
		{
			epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, it->second->getClientFD(), NULL);
			it = _connections.erase(it);
		}
		else 
			it++;
	}

	//so is it meant to be called for all or just for that fd?
}

void Webserv::setServerSocketOptions(int server_socket_fd) {

	int enable = 1;

	if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable,
		sizeof(enable)) == -1) {
	Logger::error("Setting the socket options with setsockopt() failed.");
	throw std::runtime_error("Setting socket options failed");
	}

	if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable,
		sizeof(enable)) == -1) {
	Logger::error("Setting the socket options with setsockopt() failed.");
	throw std::runtime_error("Setting socket options failed");
	}

	/// setsockopt(...), useful flags:
	/// SO_REUSEADDR, SO_REUSEPORT
}

void Webserv::setClientSocketOptions(int client_socket_fd) {
	/// setsockopt(...), useful flags:
	/// SO_KEEPALIVE, SO_RCVBUF, SO_SNDBUF, SO_RCVTIMEO, SO_RCVTIMEO

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


void Webserv::set_exit_to_true(int signal)
{
  shutdown_requested = signal;
}