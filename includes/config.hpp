#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>

// Token types for parsing config files
enum class TokenType {
    KEYWORD, 
    VALUE,
    OPEN_BRACE, 
    CLOSE_BRACE, 
    SEMICOLON,
    END_OF_FILE, 
    UNKNOWN
};

// Represents a single token with type, value, and line number
struct Token {
    TokenType type;
    std::string value;
    size_t line;
    
    Token(TokenType t, const std::string& v, size_t l) : type(t), value(v), line(l) {}
};

// Holds settings for a 'location' block
struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    bool autoindex = false;
    size_t client_max_body_size = 1048576; // Default 1MB
    std::string redirect_url;
    std::vector<std::string> allowed_methods;
    std::map<std::string, std::string> cgi_pass; // map extension to handler
    std::map<int, std::string> error_pages;
};

// Holds settings for a 'server' block
struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 80;
    std::vector<std::string> server_names;
    std::string root;
    std::string index;
    size_t client_max_body_size = 1048576; // Default 1MB
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;
};

// Holds the complete configuration with all servers
struct Config {
    std::string config_path;
    std::vector<ServerConfig> servers;
};

// Main ConfigParser namespace - public interface for parsing configuration files
namespace ConfigParser {
    // Public interface: reads a file and returns a fully parsed Config object
    Config parse(const std::string& filePath);
    
    // File validation helpers
    void validateFile(const std::string& filePath);
    bool hasValidExtension(const std::string& filePath);
    bool isEmptyFile(const std::string& filePath);
}

// The tokenizer namespace handles conversion of raw text to tokens
namespace Tokenizer {
    // Converts configuration file content into a stream of typed tokens
    std::vector<Token> tokenize(const std::string& content);
    
    // Classifies UNKNOWN tokens as KEYWORD or VALUE based on position context
    void classifyTokens(std::vector<Token>& tokens);
}


// Forward declarations for stream operators
std::ostream& operator<<(std::ostream& os, const LocationConfig& location);
std::ostream& operator<<(std::ostream& os, const ServerConfig& server);
std::ostream& operator<<(std::ostream& os, const Config& config);
