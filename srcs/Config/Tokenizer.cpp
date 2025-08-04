#include "../../includes/config.hpp"
#include <stdexcept>
#include <cctype>

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
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == ConfigParser::TokenType::UNKNOWN) {
            if (i == 0 || tokens[i-1].type == ConfigParser::TokenType::SEMICOLON ||
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