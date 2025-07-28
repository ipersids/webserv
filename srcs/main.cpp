/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 13:48:48 by jrimpila          #+#    #+#             */
/*   Updated: 2025/07/26 11:47:11 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"

static sockaddr_in init();

int handle_events(int nfds, epoll_event events[]){

	(void) nfds;
	(void) events;
	return 0;
}

void parse(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
}

int main(int argc, char *argv[])
{

	Data &data = Data::getData();

	try {
		parse(argc, argv);
	}
	catch (const std::exception& e) {
    	std::cerr << "Exception caught: " << e.what() << std::endl;
		return -1;
	}
	
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		return -1;

	sockaddr_in addr = init();
	
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//Normally, when a server closes a socket, the OS keeps the port in a TIME_WAIT state for 30–120 seconds to 
	//ensure all delayed packets are discarded. During this time, trying to bind to the same port again will fail with EADDRINUSE. 
	//Allows for a fast restart

	//seems unnecessary here because we are not trying to restart if the system calls during init fail?

	if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1){
    	return throwError("bind failed");
	}

	if (listen(server_fd, data.getMaxConnections()) == -1){
    	return throwError("listen failed");
	}

	std::vector<pollfd> fds;
    try {
		fds.push_back({server_fd, POLLIN, 0});
	}
	catch (const std::exception& e) {
    	std::cerr << "Exception caught: " << e.what() << std::endl;
		return -1;
	}

	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		return throwError("epoll_create1 failed");
	}
	epoll_event event;
	event.events = EPOLLIN; 
	event.data.fd = server_fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) 
	{
    	return throwError("epoll_ctl failed.");
	}
	epoll_event events[MaxEvents];
	
	bool serverRunning = true;
	int nfds;
	while (serverRunning)
	{
		nfds = epoll_wait(epoll_fd, events, MaxEvents, timeout);
		if (nfds == -1)
		{
			return throwError("epoll_wait failed.");
		}
		handle_events(nfds, events);
	}
}


//Now this is only listening to one port, each port needs to have a socket according to LLM
//so you would add a container that would store a list of ports
sockaddr_in init()
{
	sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
	
	return addr;
}