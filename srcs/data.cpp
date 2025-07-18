/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   data.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 14:00:20 by jrimpila          #+#    #+#             */
/*   Updated: 2025/07/18 14:56:55 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"

Data& Data::getData() {
    static Data instance;
    return instance;
}

int Data::getMaxConnections()
{
	return maxConnections;
}