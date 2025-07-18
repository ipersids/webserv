/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   data.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jrimpila <jrimpila@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 14:01:14 by jrimpila          #+#    #+#             */
/*   Updated: 2025/07/18 14:56:49 by jrimpila         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// USAGE:
//Data& data = Data::get_data();

class Data{

	public:

		static Data& getData(void);
		
		Data(const Data&) = delete;
		Data &operator=(const Data&) = delete;

		int getMaxConnections();

	private:
		Data();
		~Data();
		int maxConnections;
		
};