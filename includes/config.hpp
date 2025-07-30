#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

// Holds settings for a 'location' block
struct LocationConfig {
    std::string                 path;
    std::string                 root;
    bool                        autoindex;
    std::string                 index;
    std::vector<std::string>    allowed_methods;
    std::map<std::string, std::string> cgi_pass;
    std::map<int, std::string>  error_pages;   
    size_t                      client_max_body_size;
    std::string                 redirect_url;
    // ... other location members
};

// Holds settings for a 'server' block
struct ServerConfig {
    int                         port;
    std::string                 host;
    std::vector<std::string>    server_names;
    std::map<int, std::string>  error_pages;
    size_t                      client_max_body_size;
    std::vector<LocationConfig> locations;
};

// The main container for the entire configuration
struct Config {
    std::vector<ServerConfig>   servers;
};

class ConfigParser {
public:
    static Config parse(const std::string& filePath);

private:
    //ConfigParser(const std::string& fileContent);
    //void runParser();
};


Config DummyConfig();
void PrintConfig(const Config& config);