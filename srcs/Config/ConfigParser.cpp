#include "../../includes/config.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <cctype>
#include <vector>


namespace ConfigParser {

// Throws a runtime error with consistent formatting
void throwError(const std::string& message) {
    throw std::runtime_error("[Config Error] " + message);
}

// Checks if file has a valid configuration extension
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

// Checks if file exists but is empty or contains only whitespace
bool isEmptyFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return true;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();
    
    return content.find_first_not_of(" \t\r\n") == std::string::npos;
}

// Validates file existence, type, extension, and content
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

static void printTokens(const std::vector<Token>& tokens) {
    std::cout << "\n=== TOKENS (" << tokens.size() << " total) ===" << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& token = tokens[i];
        std::string typeStr;
        
        switch (token.type) {
            case TokenType::KEYWORD:     typeStr = "KEYWORD"; break;
            case TokenType::VALUE:       typeStr = "VALUE"; break;
            case TokenType::OPEN_BRACE:  typeStr = "OPEN_BRACE"; break;
            case TokenType::CLOSE_BRACE: typeStr = "CLOSE_BRACE"; break;
            case TokenType::SEMICOLON:   typeStr = "SEMICOLON"; break;
            case TokenType::END_OF_FILE: typeStr = "END_OF_FILE"; break;
            default:                     typeStr = "UNKNOWN"; break;
        }
        
        std::cout << "[" << i << "] " 
                  << typeStr << " "
                  << "\"" << token.value << "\" "
                  << "(line " << token.line << ")"
                  << std::endl;
    }
    std::cout << "=== END TOKENS ===" << std::endl << std::endl;
}


// Main public interface: reads file and returns complete parsed Config object
Config parse(const std::string& filePath) {
    // 1. File validation
    validateFile(filePath);
    std::cout << "File validation passed: " << filePath << std::endl;
    
    // 2. Read file content
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throwError("Failed to open file: " + filePath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    // 3. Tokenization: Convert raw text to tokens
    std::cout << "Tokenizing config file: " << filePath << std::endl;
    auto tokens = Tokenizer::tokenize(content);
    Tokenizer::classifyTokens(tokens);

    // 4. Parsing: Build config structure from tokens
    std::cout << "Parsing config file: " << filePath << std::endl;
    printTokens(tokens);
    Config config;
    //config = Parser::parseTokens(tokens);
    config.config_path = filePath;
    
    std::cout << "Config parsed successfully from: " << filePath << std::endl;
    return config;
}

} // namespace ConfigParser


namespace Tokenizer {

// Throws a runtime error with consistent formatting and optional line number
void throwError(const std::string& message, size_t line = 0) {
    std::string error = "[Tokenizer Error] ";
    if (line > 0) {
        error += "on line " + std::to_string(line) + ": ";
    }
    throw std::runtime_error(error + message);
}

// Single-pass tokenizer that correctly handles strings, comments, and special characters
std::vector<Token> tokenize(const std::string& content) {
    std::vector<Token> tokens;
    size_t line = 1;

    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\n') {
            line++;
        } else if (std::isspace(content[i])) {
            continue; // Skip whitespace
        } else if (content[i] == '#') {
            // Skip comments until end of line
            while (i < content.length() && content[i] != '\n') {
                i++;
            }
            if (i < content.length() && content[i] == '\n') line++;
        } else if (content[i] == '{') {
            tokens.push_back({TokenType::OPEN_BRACE, "{", line});
        } else if (content[i] == '}') {
            tokens.push_back({TokenType::CLOSE_BRACE, "}", line});
        } else if (content[i] == ';') {
            tokens.push_back({TokenType::SEMICOLON, ";", line});
        } else { // Keyword or Value
            std::string value;
            if (content[i] == '"' || content[i] == '\'') {
                // Handle quoted strings with proper escape sequence support
                char quote_char = content[i];
                i++; // Skip opening quote
                while (i < content.length() && content[i] != quote_char) {
                    if (content[i] == '\\' && i + 1 < content.length()) { // Handle escape character
                        value += content[++i];
                    } else {
                        value += content[i];
                    }
                    i++;
                }
                if (i >= content.length() || content[i] != quote_char) {
                    throwError("Unterminated string literal", line);
                }
            } else {
                // Handle unquoted tokens (stop at whitespace or special chars)
                while (i < content.length() && !std::isspace(content[i]) &&
                       std::string("{};#").find(content[i]) == std::string::npos) {
                    value += content[i++];
                }
                i--; // Decrement to account for the loop's ++
            }
            // The parser will determine if it's a KEYWORD or VALUE based on context
            tokens.push_back({TokenType::UNKNOWN, value, line});
        }
    }
    tokens.push_back({TokenType::END_OF_FILE, "EOF", line});
    return tokens;
}

// Determines token types based on context using simple positional heuristics
void classifyTokens(std::vector<Token>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == TokenType::UNKNOWN) {
            // A token is a KEYWORD if it's the first in a statement 
            // (i.e., after a ';' or '{' or '}' or at the start)
            if (i == 0 || tokens[i-1].type == TokenType::SEMICOLON ||
                tokens[i-1].type == TokenType::OPEN_BRACE || tokens[i-1].type == TokenType::CLOSE_BRACE) {
                tokens[i].type = TokenType::KEYWORD;
            } else {
                tokens[i].type = TokenType::VALUE;
            }
        }
    }
}

} // namespace Tokenizer




// Output stream operators for clean config printing

std::ostream& operator<<(std::ostream& os, const LocationConfig& location) {
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

std::ostream& operator<<(std::ostream& os, const ServerConfig& server) {
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

std::ostream& operator<<(std::ostream& os, const Config& config) {
    os << "=== Configuration Summary ===\n";
    os << "Config File: " << config.config_path << "\n";
    os << "Servers: " << config.servers.size() << "\n\n";
    
    for (size_t i = 0; i < config.servers.size(); ++i) {
        os << "Server " << (i + 1) << ":\n";
        os << config.servers[i] << "\n";
    }
    
    return os;
}