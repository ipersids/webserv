#pragma once

#include <string>
#include <fstream>
#include <mutex> // Required for thread safety
#include <sstream> // Required for getCurrentTimestamp


// An enum class is a more modern and type-safe alternative to a plain enum.
enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
public:
    // Delete constructor and destructor to prevent creating instances.
    Logger() = delete;
    ~Logger() = delete;
    
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void init(const std::string& log_file_path);
    static void shutdown();
    static void log(const std::string& message, LogLevel level);
    
    // Convenience methods
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

private:
    static std::ofstream    _log_file;
    static std::mutex       _log_mutex; // Mutex for thread safety
};