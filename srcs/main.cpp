/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 13:48:48 by jrimpila          #+#    #+#             */
/*   Updated: 2025/07/18 14:14:22 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"

int main(int argc, char *argv[])
{
	try {
		parse(argc, argv);
	}
	catch (const std::exception& e) {
    	std::cerr << "Exception caught: " << e.what() << std::endl;
		return -1;
	}
}