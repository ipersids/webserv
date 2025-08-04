#include "../../includes/config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace ConfigParser {

/**
 * @brief Constructor to initialize a location with inherited settings.
 * @param parent The parent ServerConfig to inherit from.
 */    
LocationConfig::LocationConfig(const ServerConfig& parent) :
    root(parent.root),
    index(parent.index),
    client_max_body_size(parent.client_max_body_size),
    cgi_pass(parent.cgi_pass),
    error_pages(parent.error_pages)
{}

void throwError(const std::string& message) {
    throw std::runtime_error("[Config Error] " + message);
}

bool hasValidExtension(const std::string& filePath) {
    const std::vector<std::string> validExtensions = {".conf", ".cfg", ".config"};
    for (const auto& ext : validExtensions) {
        if (filePath.size() >= ext.size() && 
            filePath.compare(filePath.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

bool isEmptyFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return true;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();
    
    return content.find_first_not_of(" \t\r\n") == std::string::npos;
}

/**
 * @brief Performs all pre-checks on a configuration file.
 * @param filePath The path to the configuration file.
 * @throws std::runtime_error If any validation check fails.
 */
void validateFile(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        throwError("File does not exist: " + filePath);
    }
    if (!std::filesystem::is_regular_file(filePath)) {
        throwError("Not a regular file: " + filePath);
    }
    if (!hasValidExtension(filePath)) {
        throwError("Unsupported file extension: " + filePath);
    }
    if (isEmptyFile(filePath)) {
        throwError("Empty or whitespace-only config file: " + filePath);
    }
}

// static void printTokens(const std::vector<Token>& tokens) {
//     std::cout << "\n=== TOKENS (" << tokens.size() << " total) ===" << std::endl;
//     for (size_t i = 0; i < tokens.size(); ++i) {
//         const Token& token = tokens[i];
//         std::string typeStr;
        
//         switch (token.type) {
//             case TokenType::KEYWORD:     typeStr = "KEYWORD"; break;
//             case TokenType::VALUE:       typeStr = "VALUE"; break;
//             case TokenType::OPEN_BRACE:  typeStr = "OPEN_BRACE"; break;
//             case TokenType::CLOSE_BRACE: typeStr = "CLOSE_BRACE"; break;
//             case TokenType::SEMICOLON:   typeStr = "SEMICOLON"; break;
//             case TokenType::END_OF_FILE: typeStr = "END_OF_FILE"; break;
//             default:                     typeStr = "UNKNOWN"; break;
//         }
        
//         std::cout << "[" << i << "] " 
//                   << typeStr << " "
//                   << "\"" << token.value << "\" "
//                   << "(line " << token.line << ")"
//                   << std::endl;
//     }
//     std::cout << "=== END TOKENS ===" << std::endl << std::endl;
// }

/**
 * @brief Main entry point to parse a configuration file.
 * @param filePath The path to the configuration file.
 * @return A populated Config object representing the file's contents.
 * @throws std::runtime_error On file errors or fatal parsing errors.
 */
Config parse(const std::string& filePath) {
    validateFile(filePath);
    std::cout << "File validation passed: " << filePath << std::endl;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throwError("Failed to open file: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    const std::string content = buffer.str();
    std::cout << "Tokenizing config file: " << filePath << std::endl;
    auto tokens = detail::Tokenizer::tokenize(content);
    detail::Tokenizer::classifyTokens(tokens);

    std::cout << "Parsing config file: " << filePath << std::endl;
    //printTokens(tokens);
    Config config;
    config = detail::Parser::parseTokens(tokens);
    config.config_path = filePath;
    
    std::cout << "Config parsed successfully from: " << filePath << std::endl;
    return config;
}

} // namespace ConfigParser

std::ostream& operator<<(std::ostream& os, const ConfigParser::LocationConfig& location) {
    os << "      Location: " << location.path << "\n";
    os << "        Root: " << (location.root.empty() ? "(inherited)" : location.root) << "\n";
    os << "        Index: " << (location.index.empty() ? "(inherited)" : location.index) << "\n";
    os << "        Autoindex: " << (location.autoindex ? "on" : "off") << "\n";
    os << "        Client Max Body Size: " << location.client_max_body_size << " bytes\n";
    
    if (!location.redirect_url.empty()) {
        os << "        Redirect: " << location.redirect_url << "\n";
    }
    
    if (!location.allowed_methods.empty()) {
        os << "        Allowed Methods: ";
        for (size_t i = 0; i < location.allowed_methods.size(); ++i) {
            if (i > 0) os << ", ";
            os << location.allowed_methods[i];
        }
        os << "\n";
    }
    
    if (!location.cgi_pass.empty()) {
        os << "        CGI Handlers:\n";
        for (const auto& [ext, handler] : location.cgi_pass) {
            os << "          " << ext << " -> " << handler << "\n";
        }
    }
    
    if (!location.error_pages.empty()) {
        os << "        Error Pages:\n";
        for (const auto& [code, page] : location.error_pages) {
            os << "          " << code << " -> " << page << "\n";
        }
    }
    
    return os;
}

std::ostream& operator<<(std::ostream& os, const ConfigParser::ServerConfig& server) {
    os << "  Server Configuration:\n";
    os << "    Host: " << server.host << "\n";
    os << "    Port: " << server.port << "\n";
    os << "    Root: " << (server.root.empty() ? "(not set)" : server.root) << "\n";
    os << "    Index: " << (server.index.empty() ? "(not set)" : server.index) << "\n";
    os << "    Client Max Body Size: " << server.client_max_body_size << " bytes\n";
    
    if (!server.server_names.empty()) {
        os << "    Server Names: ";
        for (size_t i = 0; i < server.server_names.size(); ++i) {
            if (i > 0) os << ", ";
            os << server.server_names[i];
        }
        os << "\n";
    }
    
    if (!server.error_pages.empty()) {
        os << "    Server Error Pages:\n";
        for (const auto& [code, page] : server.error_pages) {
            os << "      " << code << " -> " << page << "\n";
        }
    }
    
    if (!server.locations.empty()) {
        os << "    Locations:\n";
        for (const auto& location : server.locations) {
            os << location;
        }
    }
    
    return os;
}

std::ostream& operator<<(std::ostream& os, const ConfigParser::Config& config) {
    os << "=== Configuration Summary ===\n";
    os << "Config File: " << config.config_path << "\n";
    os << "Servers: " << config.servers.size() << "\n\n";
    
    for (size_t i = 0; i < config.servers.size(); ++i) {
        os << "Server " << (i + 1) << ":\n";
        os << config.servers[i] << "\n";
    }
    
    return os;
}
