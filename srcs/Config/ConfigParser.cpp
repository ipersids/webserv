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
    
    std::stringstream buffer;
    // Read the entire file into the buffer member of the config object
    buffer << file.rdbuf();

    return config;
}



Config DummyConfig()
{
    Config config;
    
    // Add a default server configuration
    ServerConfig defaultServer;
    defaultServer.host = "localhost";
    defaultServer.port = 8080;
    defaultServer.client_max_body_size = 1024 * 1024; // 1MB
    defaultServer.server_names.push_back("default_server");
    defaultServer.server_names.push_back("localhost.local");
    
    // Add server error pages
    defaultServer.error_pages[404] = "/errors/404.html";
    defaultServer.error_pages[500] = "/errors/500.html";
    
    // Add a default location that uses all fields
    LocationConfig defaultLocation;
    defaultLocation.path = "/";
    defaultLocation.root = "/var/www/html";
    defaultLocation.index = "index.html";
    defaultLocation.autoindex = true;
    defaultLocation.allowed_methods.push_back("GET");
    defaultLocation.allowed_methods.push_back("POST");
    defaultLocation.allowed_methods.push_back("DELETE");
    defaultLocation.cgi_pass[".py"] = "/usr/bin/python3";
    defaultLocation.cgi_pass[".pl"] = "/usr/bin/perl";
    defaultLocation.error_pages[403] = "/location/403.html";
    defaultLocation.error_pages[429] = "/location/429.html";
    defaultLocation.client_max_body_size = 512 * 1024; // 512KB
    defaultLocation.redirect_url = ""; // Empty for now
    
    // Add the location to the server
    defaultServer.locations.push_back(defaultLocation);
    
    // Add the server to the config
    config.servers.push_back(defaultServer);
    
    return config;
}


void PrintConfig(const Config& config)
{
    for (const auto& server : config.servers) {
        std::cout << "Server: " << server.host << std::endl;
        std::cout << "Port: " << server.port << std::endl;
        std::cout << "Max Body Size: " << server.client_max_body_size << " bytes" << std::endl;
        std::cout << "Server Names: ";
        for (const auto& name : server.server_names) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Server Error Pages: ";
        for (const auto& [code, page] : server.error_pages) {
            std::cout << code << "->" << page << " ";
        }
        std::cout << std::endl;
        
        for (const auto& location : server.locations) {
            std::cout << "  Location Path: " << location.path << std::endl;
            std::cout << "  Root: " << location.root << std::endl;
            std::cout << "  Index: " << location.index << std::endl;
            std::cout << "  Autoindex: " << (location.autoindex ? "on" : "off") << std::endl;
            std::cout << "  Client Max Body Size: " << location.client_max_body_size << " bytes" << std::endl;
            std::cout << "  Redirect URL: '" << location.redirect_url << "'" << std::endl;
            
            std::cout << "  Allowed Methods: ";
            for (const auto& method : location.allowed_methods) {
                std::cout << method << " ";
            }
            std::cout << std::endl;
            
            std::cout << "  CGI Pass: ";
            for (const auto& [ext, handler] : location.cgi_pass) {
                std::cout << ext << "->" << handler << " ";
            }
            std::cout << std::endl;
            
            std::cout << "  Location Error Pages: ";
            for (const auto& [code, page] : location.error_pages) {
                std::cout << code << "->" << page << " ";
            }
            std::cout << std::endl;
        }
    }
}
