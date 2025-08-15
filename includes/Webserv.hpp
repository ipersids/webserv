/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 13:48:02 by jrimpila          #+#    #+#             */
/*   Updated: 2025/08/15 16:01:47 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <stdexcept>

#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>



#include "macros.hpp"
#include "data.hpp"
#include "Logger.hpp"
#include "config.hpp"
#include "HttpManager.hpp"

//parsing 

constexpr int maxConstEvents = 10;

int throwError(const char *str);


class Webserv{

    public:
        Webserv() = default;
        ~Webserv() = default;
        int createConfig(char **argv);
        int createSocket();
        int createEpoll();
        void run();

    private:
        ConfigParser::Config config;
        int server_socketfd;
        HttpManager *http_manager;
        ConfigParser::ServerConfig serv_config;
        struct sockaddr_in serv_addr, cli_addr;
        int maxPendingConnections = 3; //placeholder for now
        int timeout = 10;
        int amount_fds = -1;
        int client_socketfd = -1;
        int epoll_fd;
        int maxEvents = maxConstEvents;
        size_t client_socklen;
        struct epoll_event ev, events[maxConstEvents];
        struct epoll_event client_ev;
        int bufferSize = 8192;

};