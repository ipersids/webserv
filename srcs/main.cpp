/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 13:48:48 by jrimpila          #+#    #+#             */
/*   Updated: 2025/07/18 15:01:22 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"

static sockaddr_in init();

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
    	std::cerr << "Bind failed:" << std::endl;
    	return -1;
	}

	if (listen(server_fd, data.getMaxConnections()) == -1){
    	std::cerr << "listen failed:" << std::endl;
    	return -1;
	}

	std::vector<pollfd> fds;
    try {
		fds.push_back({server_fd, POLLIN, 0});
	}
	catch (const std::exception& e) {
    	std::cerr << "Exception caught: " << e.what() << std::endl;
		return -1;
	}
	

}

sockaddr_in init()
{
	sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
	
	return addr;
}