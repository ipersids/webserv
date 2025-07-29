/**
 * @file HttpRequestUtils.hpp
 * @brief Utility functions for HTTP request processing
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 * @version 1.0
 *
 * This header provides utility functions used by the HttpRequest and
 * HttpRequestParser classes for string manipulation and HTTP-specific
 * operations.
 *
 */

#ifndef _HTTP_REQUEST_UTILS_HPP
#define _HTTP_REQUEST_UTILS_HPP

#include <algorithm>
#include <string>

/**
 * @brief Convert string to lowercase
 *
 * @param str The input string to convert
 * @return std::string A new string with all characters converted to lowercase
 *
 * @warning This function only handles ASCII characters correctly.
 */
std::string toLowerCase(const std::string& str);

#endif  // _HTTP_REQUEST_UTILS_HPP
