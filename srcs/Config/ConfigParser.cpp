#include "../../includes/config.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

// Static method to read file and start parsing
Config ConfigParser::parse(const std::string& filePath) {
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filePath);
    }

    Config config;
    
    // Read the entire file into the buffer member of the config object
    config.buffer << file.rdbuf();

    return config;
}

