#include "Config.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <set>
#include <cctype>


namespace ConfigParser {

/**
 * @brief Constructor to initialize a location with inherited settings.
 * @param parent The parent ServerConfig to inherit from.
 */    
LocationConfig::LocationConfig(const ServerConfig& parent) :
    root(parent.root),
    index(parent.index),
    client_max_body_size(parent.client_max_body_size),
    cgi_ext(parent.cgi_ext),
    cgi_path(parent.cgi_path),
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



/**
 * @brief Main entry point to parse a configuration file.
 * @param filePath The path to the configuration file.
 * @return A populated Config object representing the file's contents.
 * @throws std::runtime_error On file errors or fatal parsing errors.
 */
Config parse(const std::string& filePath) {
    validateFile(filePath);
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throwError("Failed to open file: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    const std::string content = buffer.str();
    auto tokens = detail::Tokenizer::tokenize(content);
    detail::Tokenizer::classifyTokens(tokens);

    Config config;
    config = detail::Parser::parseTokens(tokens);
    config.config_path = filePath;

    return config;
}

} // namespace ConfigParser




namespace ConfigParser::detail::Tokenizer {

/**
 * @brief Throw formatted tokenizer error with line number
 * @param message Error message
 * @param line Line number where error occurred
 */
void throwError(const std::string& message, size_t line) {
    std::string error = "[Tokenizer Error] ";
    if (line > 0) {
        error += "on line " + std::to_string(line) + ": ";
    }
    throw std::runtime_error(error + message);
}

/**
 * @brief Convert config text into tokens
 * @param content Configuration file content
 * @return Vector of tokens with line numbers
 */
std::vector<ConfigParser::Token> tokenize(const std::string& content) {
    std::vector<ConfigParser::Token> tokens;
    size_t line = 1;

    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\n') {
            line++;
            continue; // Treat newline as whitespace only
        } else if (std::isspace(content[i])) {
            continue; // Skip whitespace
        } else if (content[i] == '#') {
            // Skip comments until end of line
            while (i < content.length() && content[i] != '\n') {
                i++;
            }
            if (i < content.length() && content[i] == '\n') line++;
        } else if (content[i] == '{') {
            tokens.push_back({ConfigParser::TokenType::OPEN_BRACE, "{", line});
        } else if (content[i] == '}') {
            tokens.push_back({ConfigParser::TokenType::CLOSE_BRACE, "}", line});
        } else if (content[i] == ';') {
            tokens.push_back({ConfigParser::TokenType::SEMICOLON, ";", line});
        } else { // Keyword or Value
            std::string value;
            if (content[i] == '"' || content[i] == '\'') {
                char quote_char = content[i];
                i++;
                while (i < content.length() && content[i] != quote_char) {
                    if (content[i] == '\\' && i + 1 < content.length()) {
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
                while (i < content.length() && !std::isspace(content[i]) &&
                       std::string("{};#").find(content[i]) == std::string::npos) {
                    value += content[i++];
                }
                i--;
            }
            tokens.push_back({ConfigParser::TokenType::UNKNOWN, value, line});
        }
    }
    tokens.push_back({ConfigParser::TokenType::END_OF_FILE, "EOF", line});
    return tokens;
}

/**
 * @brief Classify tokens as KEYWORD or VALUE based on position
 * @param tokens Vector of tokens to classify
 */
void classifyTokens(std::vector<ConfigParser::Token>& tokens) {
    // List of known directive names (should match Parser.cpp isValidDirective)
    static const std::unordered_set<std::string> known_directives = {
        "http", "server", "location", "include", "worker_processes", "worker_connections",
        "sendfile", "listen", "port", "host", "server_name", "root", "index",
        "error_page", "client_max_body_size", "autoindex", "allow_methods",
        "cgi_pass", "return", "cgi_path", "cgi_ext"
    };
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == ConfigParser::TokenType::UNKNOWN) {
            if (known_directives.count(tokens[i].value)) {
                tokens[i].type = ConfigParser::TokenType::KEYWORD;
            } else if (i == 0 || tokens[i-1].type == ConfigParser::TokenType::SEMICOLON ||
                       tokens[i-1].type == ConfigParser::TokenType::OPEN_BRACE ||
                       tokens[i-1].type == ConfigParser::TokenType::CLOSE_BRACE) {
                tokens[i].type = ConfigParser::TokenType::KEYWORD;
            } else {
                tokens[i].type = ConfigParser::TokenType::VALUE;
            }
        }
    }
}

} // namespace ConfigParser::detail::Tokenizer


namespace ConfigParser::detail::Parser {

/**
 * @brief Throw a formatted parser error with line number information
 * @param message Error message to include in the exception
 * @param line Line number where the error occurred (0 for no line info)
 * @throws std::runtime_error with formatted message and line number
 */
void throwError(const std::string& message, size_t line) {
    std::string error = "[Parser Error] ";
    if (line > 0) {
        error += "on line " + std::to_string(line) + ": ";
    }
    throw std::runtime_error(error + message);
}

/**
 * @brief Write a formatted warning message to stderr
 * @param message Warning message to output
 * @param line Line number where the warning occurred
 */
void writeWarning(const std::string& message, size_t line) {
    std::cerr << "[Parser Warning] on line " << line << ": " << message << std::endl;
}

/**
 * @brief Parse body size value with optional K/M suffixes
 * @param value String value to parse (e.g., "10M", "512K", "1024")
 * @return Size in bytes
 * @throws std::runtime_error if value format is invalid
 */
size_t parseBodySize(const std::string& value) {
    if (value.empty()) {
        throw std::runtime_error("Empty body size value");
    }
    
    std::string numStr = value;
    size_t multiplier = 1;
    
    char lastChar = value.back();
    if (lastChar == 'K' || lastChar == 'k') {
        multiplier = 1024;
        numStr = value.substr(0, value.length() - 1);
    } else if (lastChar == 'M' || lastChar == 'm') {
        multiplier = 1024 * 1024;
        numStr = value.substr(0, value.length() - 1);
    }
    
    try {
        size_t size = std::stoull(numStr);
        return size * multiplier;
    } catch (...) {
        throw std::runtime_error("Invalid body size format: " + value);
    }
}

/**
 * @brief Validate that port number is within acceptable range
 * @param port Port number to validate
 * @throws std::runtime_error if port is not between 1-65535
 */
void validatePort(int port) {
    if (port < 1 || port > 65535) {
        throw std::runtime_error("Port must be between 1 and 65535, got: " + std::to_string(port));
    }
}

/**
 * @brief Validate that server has mandatory root location "/"
 * @param server ServerConfig object to validate
 * @throws std::runtime_error if server doesn't have a root location
 */
void validateServerHasRootLocation(const ConfigParser::ServerConfig& server) {
    bool hasRootLocation = false;
    for (const auto& location : server.locations) {
        if (location.path == "/") {
            hasRootLocation = true;
            break;
        }
    }
    
    if (!hasRootLocation) {
        throwError("Server block must have a root location '/' - required by nginx specification", 0);
    }
}

/**
 * @brief Validate CGI configuration in a location block
 * @param location LocationConfig object to validate
 * @throws std::runtime_error if CGI configuration is invalid
 */
void validateLocationCgiConfig(const ConfigParser::LocationConfig& location) {
    // Check that cgi_ext and cgi_path have matching sizes for proper pairing
    if (!location.cgi_ext.empty() || !location.cgi_path.empty()) {
        if (location.cgi_ext.size() != location.cgi_path.size()) {
            throwError("Number of cgi_ext entries (" + std::to_string(location.cgi_ext.size()) + 
                      ") must match number of cgi_path entries (" + std::to_string(location.cgi_path.size()) + 
                      ") for proper extension-interpreter pairing in location '" + location.path + "'", 0);
        }
    }
}

/**
 * @brief Check if directive name is in the list of known/valid directives
 * @param directive Directive name to check
 * @return true if directive is recognized as valid
 */
bool isValidDirective(const std::string& directive) {
    static const std::unordered_set<std::string> valid = {
        "http", "server", "location", "include", "worker_processes", "worker_connections",
        "sendfile", "listen", "port", "host", "server_name", "root", "index",
        "error_page", "client_max_body_size", "autoindex", "allow_methods",
        "cgi_pass", "return", "cgi_path", "cgi_ext"
    };
    return valid.count(directive);
}

/**
 * @brief Check if directive is valid in server context
 * @param directive Directive name to check
 * @return true if directive can be used inside server blocks
 */
bool isValidInServerContext(const std::string& directive) {
    static const std::unordered_set<std::string> valid = {
        "listen", "server_name", "host", "root", "index", "error_page",
        "client_max_body_size", "cgi_path", "port"
    };
    return valid.count(directive);
}

/**
 * @brief Check if directive is valid in location context
 * @param directive Directive name to check
 * @return true if directive can be used inside location blocks
 */
bool isValidInLocationContext(const std::string& directive) {
    static const std::unordered_set<std::string> valid = {
        "root", "index", "autoindex", "allow_methods", "methods", "return",
        "cgi_path", "cgi_ext", "error_page", "client_max_body_size"
    };
    return valid.count(directive);
}

/**
 * @brief Helper to consume a directive's values and its trailing semicolon
 * @param keyword_name Name of the directive being parsed (for error messages)
 * @param keyword_line Line number of the directive (for error messages)
 * @param tokens Vector of tokens being parsed
 * @param pos Current position in tokens (modified by reference)
 * @return Vector of string values found for the directive
 * @throws std::runtime_error if semicolon is missing or syntax error detected
 */
static std::vector<std::string> getDirectiveValues(const std::string& keyword_name, size_t keyword_line, const std::vector<ConfigParser::Token>& tokens, size_t& pos) {
    std::vector<std::string> values;
    while (pos < tokens.size() && tokens[pos].type == ConfigParser::TokenType::VALUE) {
        values.push_back(tokens[pos].value);
        pos++;
    }

    // If the next token is a VALUE, that means a semicolon was missing between directives
    if (pos < tokens.size() && tokens[pos].type == ConfigParser::TokenType::VALUE) {
        throwError("Directive '" + keyword_name + "' must end with a semicolon ';' (missing before value '" + tokens[pos].value + "')", keyword_line);
    }

    if (pos < tokens.size() && tokens[pos].type == ConfigParser::TokenType::KEYWORD) {
        throwError("Directive '" + keyword_name + "' must end with a semicolon ';' - found '" + 
                  tokens[pos].value + "' instead", keyword_line);
    }
    
    if (pos >= tokens.size() || tokens[pos].type != ConfigParser::TokenType::SEMICOLON) {
        throwError("Directive '" + keyword_name + "' must end with a semicolon ';'", keyword_line);
    }
    pos++; // Consume ';'
    return values;
}
/**
 * @brief Parse individual server-level directives and populate ServerConfig
 * @param server ServerConfig object to populate with directive values
 * @param tokens Vector of tokens being parsed
 * @param pos Current position in tokens (modified by reference)
 * @throws std::runtime_error if directive is invalid, malformed, or has wrong context
 */
void parseServerDirective(ConfigParser::ServerConfig& server, const std::vector<ConfigParser::Token>& tokens, size_t& pos) {
    const ConfigParser::Token& keyword = tokens[pos];
    pos++; // Consume keyword
    
    if (!isValidDirective(keyword.value)) {
        writeWarning("Unknown directive '" + keyword.value + "'", keyword.line);
    }
    
    if (!isValidInServerContext(keyword.value)) {
        throwError("Directive '" + keyword.value + "' not allowed in server context", keyword.line);
    }

    // Use the helper to get the values
    std::vector<std::string> values = getDirectiveValues(keyword.value, keyword.line, tokens, pos);
    
    // Populate ServerConfig based on the directive
    if ((keyword.value == "listen" && !values.empty()) || (keyword.value == "port" && !values.empty())) {
        try { 
            server.port = std::stoi(values[0]);
            validatePort(server.port);
        }
        catch (...) { 
            throwError("Invalid port number '" + values[0] + "'", keyword.line); 
        }
    } else if (keyword.value == "server_name") {
        server.server_names = values;
    } else if (keyword.value == "host" && !values.empty()) {
        server.host = values[0];
    } else if (keyword.value == "root" && !values.empty()) {
        server.root = values[0];
    } else if (keyword.value == "index" && !values.empty()) {
        server.index = values[0];
    } else if (keyword.value == "client_max_body_size" && !values.empty()) {
        try {
            server.client_max_body_size = parseBodySize(values[0]);
        }
        catch (...) {
            throwError("Invalid client_max_body_size '" + values[0] + "'", keyword.line);
        }
    } else if (keyword.value == "error_page" && values.size() >= 2) {
        try {
            int code = std::stoi(values[0]);
            server.error_pages[code] = values[1];
        }
        catch (...) {
            throwError("Invalid error_page format", keyword.line);
        }
    } else if (keyword.value == "cgi_path" && !values.empty()) {
        server.cgi_path = values;
    } else if (keyword.value == "cgi_ext" && !values.empty()) {
        server.cgi_ext = values;
    }
}

/**
 * @brief Parse individual location-level directives and populate LocationConfig
 * @param location LocationConfig object to populate with directive values
 * @param tokens Vector of tokens being parsed
 * @param pos Current position in tokens (modified by reference)
 * @throws std::runtime_error if directive is invalid, malformed, or has wrong context
 */
void parseLocationDirective(ConfigParser::LocationConfig& location, const std::vector<ConfigParser::Token>& tokens, size_t& pos) {
    const ConfigParser::Token& keyword = tokens[pos];
    pos++; // Consume keyword
    
    if (!isValidDirective(keyword.value)) {
        writeWarning("Unknown directive '" + keyword.value + "'", keyword.line);
    }
    
    if (!isValidInLocationContext(keyword.value)) {
        throwError("Directive '" + keyword.value + "' not allowed in location context", keyword.line);
    }

    std::vector<std::string> values = getDirectiveValues(keyword.value, keyword.line, tokens, pos);
    
    if (keyword.value == "root" && !values.empty()) {
        location.root = values[0];
    } else if (keyword.value == "index" && !values.empty()) {
        location.index = values[0];
    } else if (keyword.value == "autoindex" && !values.empty()) {
        location.autoindex = (values[0] == "on" || values[0] == "true");
    } else if (keyword.value == "allow_methods" || keyword.value == "methods") {
        location.allowed_methods = values;
    } else if (keyword.value == "return" && !values.empty()) {
        location.redirect_url = values[0];
    } else if (keyword.value == "client_max_body_size" && !values.empty()) {
        try {
            location.client_max_body_size = parseBodySize(values[0]);
        }
        catch (...) {
            throwError("Invalid client_max_body_size '" + values[0] + "'", keyword.line);
        }
    } else if (keyword.value == "error_page" && values.size() >= 2) {
        try {
            int code = std::stoi(values[0]);
            location.error_pages[code] = values[1];
        }
        catch (...) {
            throwError("Invalid error_page format", keyword.line);
        }
    } else if (keyword.value == "cgi_path" && !values.empty()) {
        location.cgi_path = values;
    } else if (keyword.value == "cgi_ext" && !values.empty()) {
        location.cgi_ext = values;
    }
}

/**
 * @brief Parse a location block within a server block
 * @param server ServerConfig object to add the parsed location to
 * @param tokens Vector of tokens being parsed
 * @param pos Current position in tokens (modified by reference)
 * @throws std::runtime_error if location syntax is invalid or malformed
 */
void parseLocationBlock(ConfigParser::ServerConfig& server, const std::vector<ConfigParser::Token>& tokens, size_t& pos) {
    ConfigParser::LocationConfig location(server);
    pos++; // Consume "location"
    
    if (pos < tokens.size() && tokens[pos].type == ConfigParser::TokenType::VALUE) {
        location.path = tokens[pos].value;
        pos++;
    } else {
        throwError("Expected path for location block", tokens[pos-1].line);
    }

    if (pos >= tokens.size() || tokens[pos].type != ConfigParser::TokenType::OPEN_BRACE) {
        throwError("Expected '{' after location path", tokens[pos-1].line);
    }
    pos++; // Consume '{'
    
    // Parse directives inside location block
    while (pos < tokens.size() && tokens[pos].type != ConfigParser::TokenType::CLOSE_BRACE) {
        if (tokens[pos].type != ConfigParser::TokenType::KEYWORD) {
            throwError("Expected a directive keyword", tokens[pos].line);
        }
        parseLocationDirective(location, tokens, pos);
    }

    
    if (pos >= tokens.size() || tokens[pos].type != ConfigParser::TokenType::CLOSE_BRACE) {
         throwError("Expected '}' to close 'location' block", tokens.back().line);
    }
    pos++; // Consume '}'
    validateLocationCgiConfig(location);
    server.locations.push_back(location);
}

/**
 * @brief Parse a complete server block and add it to the config
 * @param config Config object to add the parsed server to
 * @param tokens Vector of tokens being parsed
 * @param pos Current position in tokens (modified by reference)
 * @throws std::runtime_error if server syntax is invalid or missing root location
 */
void parseServerBlock(ConfigParser::Config& config, const std::vector<ConfigParser::Token>& tokens, size_t& pos) {
    ConfigParser::ServerConfig server;
    pos++; // Consume "server" keyword

    if (pos >= tokens.size() || tokens[pos].type != ConfigParser::TokenType::OPEN_BRACE) {
        throwError("Expected '{' after 'server' keyword", tokens[pos].line);
    }
    pos++; // Consume '{'

    while (pos < tokens.size() && tokens[pos].type != ConfigParser::TokenType::CLOSE_BRACE) {
        if (tokens[pos].type != ConfigParser::TokenType::KEYWORD) {
            throwError("Expected a directive keyword", tokens[pos].line);
        }
        
        if (tokens[pos].value == "location") {
            parseLocationBlock(server, tokens, pos);
        } else {
            parseServerDirective(server, tokens, pos);
        }
    }

    if (pos >= tokens.size() || tokens[pos].type != ConfigParser::TokenType::CLOSE_BRACE) {
        throwError("Expected '}' to close 'server' block", tokens.back().line);
    }
    pos++; // Consume '}'
    
    validateServerHasRootLocation(server);
    config.servers.push_back(server);
}

void final_validation(ConfigParser::Config& config) {
    // Check for duplicate server blocks with same port and server_name
    std::set<std::pair<int, std::string>> seen;
    for (const auto& server : config.servers) {
        for (const auto& name : server.server_names) {
            auto key = std::make_pair(server.port, name);
            if (seen.count(key)) {
                throw std::runtime_error("Duplicate server block for port " + std::to_string(server.port) + " and server_name " + name);
            }
            seen.insert(key);
        }
    }
}

/**
 * @brief Main entry point for parsing token stream into Config object
 * @param tokens Vector of tokens to parse (modified during parsing)
 * @return Complete Config object with all parsed servers
 * @throws std::runtime_error if parsing fails due to syntax errors
 */
ConfigParser::Config parseTokens(std::vector<ConfigParser::Token>& tokens) {
    ConfigParser::Config config;
    
    size_t pos = 0;
    while(pos < tokens.size() && tokens[pos].type != ConfigParser::TokenType::END_OF_FILE) {
        if (tokens[pos].type == ConfigParser::TokenType::KEYWORD && tokens[pos].value == "server") {
            parseServerBlock(config, tokens, pos);
        } else {
            writeWarning("Unexpected token '" + tokens[pos].value + "' at top level", tokens[pos].line);
            pos++;
        }
    }
    final_validation(config);
    return config;
}

} // namespace ConfigParser::detail::Parser



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
    
    if (!location.cgi_ext.empty() && !location.cgi_path.empty()) {
        os << "        CGI Handlers:\n";
        size_t count = std::min(location.cgi_ext.size(), location.cgi_path.size());
        for (size_t i = 0; i < count; ++i) {
            os << "          " << location.cgi_ext[i] << " -> " << location.cgi_path[i] << "\n";
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
