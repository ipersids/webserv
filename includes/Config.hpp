#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace ConfigParser {

    enum class TokenType {
        UNKNOWN,
        KEYWORD, 
        VALUE,
        OPEN_BRACE, 
        CLOSE_BRACE, 
        SEMICOLON,
        END_OF_FILE
    };

    struct Token {
        TokenType   type;
        std::string value;
        size_t      line;
    };

    struct ServerConfig;

    struct LocationConfig {
        std::string path;
        std::string root;
        std::string index;
        bool        autoindex               = false;
        size_t      client_max_body_size    = 1048576; // Default 1MB
        std::string redirect_url;
        std::vector<std::string>            allowed_methods;
        std::vector<std::string>            cgi_ext;
        std::vector<std::string>            cgi_path;
        std::map<int, std::string>          error_pages;

        LocationConfig(const ServerConfig& parent);
    };

    struct ServerConfig {
        std::string host                    = "0.0.0.0";
        int         port                    = 80;
        std::vector<std::string>            server_names;
        std::string root;
        std::string index;
        size_t      client_max_body_size    = 1048576; // Default 1MB
        std::map<int, std::string>          error_pages;
        std::vector<std::string>            cgi_ext;
        std::vector<std::string>            cgi_path;
        std::vector<LocationConfig>         locations;
    };

    struct Config {
        std::string                 config_path;
        std::vector<ServerConfig>   servers;
    };


    Config parse(const std::string& filePath);

    namespace detail {

        namespace Tokenizer {
            std::vector<Token> tokenize(const std::string& content);
            void classifyTokens(std::vector<Token>& tokens);
        }

        namespace Parser {
            Config parseTokens(std::vector<Token>& tokens);
            
            void parseServerBlock(Config& config, const std::vector<Token>& tokens, size_t& pos);
            void parseLocationBlock(ServerConfig& server, const std::vector<Token>& tokens, size_t& pos);

            void parseServerDirective(ServerConfig& server, const std::vector<Token>& tokens, size_t& pos);
            void parseLocationDirective(LocationConfig& location, const std::vector<Token>& tokens, size_t& pos);

            size_t parseBodySize(const std::string& value);
            void validatePort(int port);
            bool isValidDirective(const std::string& directive);
            bool isValidInServerContext(const std::string& directive);
            bool isValidInLocationContext(const std::string& directive);
            void throwError(const std::string& message, size_t line = 0);
            void writeWarning(const std::string& message, size_t line);
        }

    } // namespace detail
} // namespace ConfigParser

std::ostream& operator<<(std::ostream& os, const ConfigParser::LocationConfig& location);
std::ostream& operator<<(std::ostream& os, const ConfigParser::ServerConfig& server);
std::ostream& operator<<(std::ostream& os, const ConfigParser::Config& config);
