/**
 * @file HttpRequestUtils.cpp
 * @brief Implementation of utility functions for HTTP request processing
 * @author Julia Persidskaia (ipersids)
 * @date 2025-07-28
 */

#include "HttpRequestUtils.hpp"

std::string toLowerCase(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}
