#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <filesystem>

std::ofstream Logger::_log_file;
std::mutex    Logger::_log_mutex;

/**
 * @brief Get the log level as a string
 * @param level The log level to convert
 * @return String representation of the log level
 */
static std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
    }
    return "UNKNOWN";
}

/**
 * @brief Get the current timestamp as a formatted string
 * @return Formatted timestamp string (YYYY-MM-DD HH:MM:SS)
 */
static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::tm time_info;
    localtime_r(&in_time_t, &time_info);

    std::stringstream ss;
    ss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * @brief Initialize the logger with a specified log file path
 * @param log_file_path Path to the log file (will be created or appended to)
 * @throws std::runtime_error if the file cannot be opened
 */
void Logger::init(const std::string& log_file_path) {
    std::lock_guard<std::mutex> lock(_log_mutex);
    
    try {
        std::filesystem::path path(log_file_path);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        _log_file.open(log_file_path, std::ios::app);
        if (!_log_file.is_open()) {
            throw std::runtime_error("Logger::init failed to open log file: " + log_file_path);
        }

        _log_file << "[" << levelToString(LogLevel::Info) << "] "
                  << getCurrentTimestamp() << ": --- Logger Initialized ---" << '\n';
        _log_file.flush();

    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Logger::init failed to create log directory: " + std::string(e.what()));
    }
}

/**
 * @brief Shutdown the logger and close the log file
 * Writes a shutdown message and properly closes the file stream
 */
void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(_log_mutex);
    
    if (_log_file.is_open()) {
        _log_file << "[" << levelToString(LogLevel::Info) << "] "
                  << getCurrentTimestamp() << ": --- Logger Shutdown ---" << std::endl;
        _log_file.close();
    }
}

/**
 * @brief Log a message with the specified level
 * @param message The message to log
 * @param level The log level (Info, Warning, Error)
 * @note Error messages are also printed to stderr
 * @note Warning and Error messages are flushed immediately
 */
void Logger::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(_log_mutex);

    if (!_log_file.is_open()) {
        std::cerr << "Logger not initialized. Message: " << message << std::endl;
        return;
    }
    _log_file << "[" << levelToString(level) << "] "
              << getCurrentTimestamp() << ": " << message << std::endl;
    if (level == LogLevel::Error || level == LogLevel::Warning) {
        _log_file.flush();
    }
    if (level == LogLevel::Error) {
        std::cerr << "[ERROR]: " << message << std::endl;
    }
}

/**
 * @brief Log an info message
 * @param message The message to log
 */
void Logger::info(const std::string& message) {
    log(message, LogLevel::Info);
}

/**
 * @brief Log a warning message
 * @param message The message to log
 */
void Logger::warning(const std::string& message) {
    log(message, LogLevel::Warning);
}

/**
 * @brief Log an error message
 * @param message The message to log
 */
void Logger::error(const std::string& message) {
    log(message, LogLevel::Error);
}