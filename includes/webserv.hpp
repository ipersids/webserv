/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 13:48:02 by jrimpila          #+#    #+#             */
/*   Updated: 2025/07/18 14:24:29 by jrimpila         ###   ########.fr       */
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

#include "macros.hpp"
#include "data.hpp"

//parsing 

void parse(int argc, char *argv[]);