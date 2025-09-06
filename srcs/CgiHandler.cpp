#include "../includes/CgiHandler.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <poll.h>
#include <signal.h>


/**
 * @brief Determines if a request should be handled by CGI.
 *
 * Checks if the file path resolves to a file with an extension that matches the configured CGI extensions.
 * This function handles both direct file requests and directory requests with index files.
 *
 * @param file_path The validated file system path to check.
 * @param location The matched location configuration block.
 * @return true if the request should be handled by CGI, false otherwise.
 */
bool CgiHandler::isCgiRequest(const std::string& file_path, const ConfigParser::LocationConfig& location) {
    if (location.cgi_ext.empty()) {
        return false;
    }
    
    std::string script_path = file_path;
    
    // If the path is a directory, check for index file
    if (std::filesystem::exists(file_path) && std::filesystem::is_directory(file_path)) {
        if (!location.index.empty()) {
            std::string index_path = file_path;
            if (!index_path.empty() && index_path.back() != '/') {
                index_path += '/';
            }
            index_path += location.index;
            
            if (std::filesystem::exists(index_path) && std::filesystem::is_regular_file(index_path)) {
                script_path = index_path; // Use index file for extension check
            } else {
                return false; // No valid index file
            }
        } else {
            return false; // No index configured for directory
        }
    }
    
    // Extract file extension
    size_t dot_pos = script_path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false; // No extension
    }
    
    std::string extension = script_path.substr(dot_pos);
    
    // Check if extension is in the cgi_ext vector
    return std::find(location.cgi_ext.begin(), location.cgi_ext.end(), extension) != location.cgi_ext.end();
}

/**
 * @brief Main entry point for executing a CGI script.
 *
 * This function assumes all prior validations have been performed by HttpMethodHandler:
 * - Location exists and is valid
 * - Request body size is within limits
 * - Method is allowed
 * - File path security has been validated
 * - CGI extension check has been performed
 * 
 * This function focuses on CGI execution and handles:
 * 1. Resolving the script path from file_path (including index files for directories).
 * 2. Validating script existence.
 * 3. Getting the interpreter for the script.
 * 4. Setting up pipes and executing the CGI script.
 * 5. Handling timeout and process management.
 * 6. Parsing and returning the CGI output.
 *
 * @param request The validated HTTP request object.
 * @param location The validated location configuration block.
 * @param file_path The validated file system path.
 * @return HttpResponse The response from the CGI script or an error response.
 */
HttpResponse CgiHandler::execute(const HttpRequest& request, const ConfigParser::LocationConfig& location, const std::string& file_path) {
    std::string script_path = file_path;
    
    // If the path is a directory, resolve to index file
    if (std::filesystem::exists(file_path) && std::filesystem::is_directory(file_path)) {
        if (!location.index.empty()) {
            std::string index_path = file_path;
            if (!index_path.empty() && index_path.back() != '/') {
                index_path += '/';
            }
            index_path += location.index;
            
            if (std::filesystem::exists(index_path) && std::filesystem::is_regular_file(index_path)) {
                script_path = index_path;
            } else {
                return createErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND, "Index file not found: " + location.index);
            }
        } else {
            return createErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND, "Directory access not allowed");
        }
    }

    // Final validation: ensure script exists and is a regular file
    if (!std::filesystem::exists(script_path) || !std::filesystem::is_regular_file(script_path)) {
        return createErrorResponse(HttpUtils::HttpStatusCode::NOT_FOUND, "CGI script not found: " + script_path);
    }
    
    std::string interpreter = getInterpreter(script_path, location);
    if (interpreter.empty()) {
        return createErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR, "No interpreter configured for script: " + script_path);
    }

    // Setup pipes for communication
    int input_pipe[2], output_pipe[2];
    if (pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
        return createErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR, "Pipe creation failed");
    }

    // Execute CGI script
    return executeCgiScript(request, script_path, interpreter, input_pipe, output_pipe);
}

/**
 * @brief Executes the CGI script in a child process with timeout handling.
 *
 * This method handles the core CGI execution logic:
 * 1. Writes request body to the script's stdin.
 * 2. Forks and executes the CGI script.
 * 3. Reads script output with timeout protection.
 * 4. Cleans up processes and file descriptors.
 *
 * @param request The HTTP request object.
 * @param script_path Path to the CGI script.
 * @param interpreter Path to the script interpreter.
 * @param input_pipe Pipe for sending data to CGI script.
 * @param output_pipe Pipe for receiving data from CGI script.
 * @return HttpResponse The response from the CGI script.
 */
HttpResponse CgiHandler::executeCgiScript(const HttpRequest& request, const std::string& script_path, 
                                        const std::string& interpreter, int input_pipe[2], int output_pipe[2]) {
    // Write request body to script's stdin
    const std::string& body = request.getBody();
    if (!body.empty()) {
        write(input_pipe[1], body.c_str(), body.length());
    }
    close(input_pipe[1]);

    auto envp = setupEnvironment(request, script_path);
    
    // Fork and execute CGI script
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        close(input_pipe[0]);
        close(output_pipe[1]);

        const char* args[] = { interpreter.c_str(), script_path.c_str(), nullptr };
        execve(interpreter.c_str(), const_cast<char* const*>(args), envp.get());
        perror("CGI execve failed");
        _exit(EXIT_FAILURE);
    }
    
    if (pid == -1) {
        close(input_pipe[0]);
        close(output_pipe[0]);
        close(output_pipe[1]);
        return createErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR, "Fork failed");
    }
    
    // Parent process - close unused pipe ends
    close(input_pipe[0]);
    close(output_pipe[1]);
    
    // Read CGI output with timeout
    std::string output = readCgiOutput(output_pipe[0], pid);
    close(output_pipe[0]);
    
    // Wait for child process to complete
    int status;
    waitpid(pid, &status, 0);
    
    if (output.empty()) {
        return createErrorResponse(HttpUtils::HttpStatusCode::INTERNAL_SERVER_ERROR, "CGI execution failed");
    }
    
    return parseOutput(output);
}

/**
 * @brief Reads CGI script output with timeout protection.
 *
 * Uses poll() to implement a timeout mechanism while reading from the CGI script.
 * If the script takes too long or hangs, it will be killed and an empty string returned.
 *
 * @param output_fd File descriptor to read CGI output from.
 * @param pid Process ID of the CGI script (for cleanup on timeout).
 * @return std::string The output from the CGI script, or empty on timeout/error.
 */
std::string CgiHandler::readCgiOutput(int output_fd, pid_t pid) {
    struct pollfd pfd;
    pfd.fd = output_fd;
    pfd.events = POLLIN;
    
    std::string output;
    char buffer[CGI_BUFSIZE];
    int timeout_ms = CGI_TIMEOUT * 1000;
    
    while (true) {
        int poll_result = poll(&pfd, 1, timeout_ms);
        
        if (poll_result == 0) {
            // Timeout - kill the process
            kill(pid, SIGKILL);
            return ""; // Will be handled as execution failed
        } else if (poll_result < 0) {
            // Poll error - kill the process
            kill(pid, SIGKILL);
            return ""; // Will be handled as execution failed
        }
        
        ssize_t bytes_read = read(output_fd, buffer, CGI_BUFSIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            output += buffer;
        } else if (bytes_read == 0) {
            break; // End of file
        } else {
            break; // Read error
        }
    }
    
    return output;
}

HttpResponse CgiHandler::createErrorResponse(HttpUtils::HttpStatusCode status, const std::string& message) {
    HttpResponse response;
    response.setErrorResponse(status, message);
    return response;
}

/**
 * @brief Gets the interpreter path for a CGI script based on its extension.
 *
 * Extracts the file extension from the script path and finds the
 * corresponding interpreter in the location's CGI configuration.
 * This function assumes the extension exists in cgi_ext (pre-validated by isCgiRequest).
 *
 * @param script_path The full filesystem path to the CGI script.
 * @param location The location configuration containing CGI interpreter mappings.
 * @return std::string The path to the interpreter, or empty string if not found.
 */
std::string CgiHandler::getInterpreter(const std::string& script_path, const ConfigParser::LocationConfig& location) {
    size_t dot_pos = script_path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string extension = script_path.substr(dot_pos);
    for (size_t i = 0; i < location.cgi_ext.size() && i < location.cgi_path.size(); ++i) {
        if (location.cgi_ext[i] == extension) {
            return location.cgi_path[i];
        }
    }
    return "";
}

/**
 * @brief Parses CGI script output into an HTTP response object.
 *
 * Separates CGI headers from the response body and processes them according
 * to CGI specification. Handles both explicit Status headers and default
 * 200 OK responses. Supports both \r\n\r\n and \n\n header separators.
 *
 * @param output The raw output string from the CGI script.
 * @return HttpResponse The parsed HTTP response with headers and body.
 */
HttpResponse CgiHandler::parseOutput(const std::string& output) {
    HttpResponse response;
    size_t header_end = output.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = output.find("\n\n");
    }
    
    if (header_end != std::string::npos) {
        std::string headers = output.substr(0, header_end);
        std::string response_body = output.substr(header_end + (output.find("\r\n\r\n") != std::string::npos ? 4 : 2));
        
        // Parse headers
        std::istringstream header_stream(headers);
        std::string line;
        bool has_status = false;
        
        while (std::getline(header_stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                
                if (name == "Status") {
                    has_status = true;
                    int status_code = std::stoi(value.substr(0, 3));
                    response.setStatusCode(static_cast<HttpUtils::HttpStatusCode>(status_code));
                } else {
                    response.insertHeader(name, value);
                }
            }
        }
        
        if (!has_status) {
            response.setStatusCode(HttpUtils::HttpStatusCode::OK);
        }
        response.setBody(response_body);
    } else {
        // No headers, treat as plain output
        response.setStatusCode(HttpUtils::HttpStatusCode::OK);
        response.setBody(output);
    }
    
    return response;
}

/**
 * @brief Converts a string map to a C-style environment array for execve().
 *
 * Creates a null-terminated array of C strings in "KEY=VALUE" format
 * suitable for passing to execve(). Uses a custom deleter to ensure
 * proper memory cleanup of both the array and individual strings.
 *
 * @param env_map Map of environment variable names to values.
 * @return std::unique_ptr<char*[]> Smart pointer managing the environment array.
 */
static std::unique_ptr<char*[], void(*)(char**)> mapToEnvArray(const std::map<std::string, std::string>& env_map) {
    auto deleter = [](char** envp) {
        if (envp) {
            for (int i = 0; envp[i]; i++) {
                delete[] envp[i];
            }
            delete[] envp;
        }
    };
    
    char** raw_envp = new char*[env_map.size() + 1];
    int i = 0;
    for (const auto& pair : env_map) {
        std::string env_string = pair.first + "=" + pair.second;
        raw_envp[i] = new char[env_string.length() + 1];
        std::strcpy(raw_envp[i], env_string.c_str());
        i++;
    }
    raw_envp[i] = nullptr;
    
    return std::unique_ptr<char*[], void(*)(char**)>(raw_envp, deleter);
}

/**
 * @brief Sets up the CGI environment variables from the HTTP request.
 *
 * Creates a comprehensive set of CGI environment variables according to
 * the CGI/1.1 specification, including:
 * - Standard CGI variables (REQUEST_METHOD, QUERY_STRING, etc.)
 * - Server information (SERVER_NAME, SERVER_PORT, etc.)
 * - HTTP headers converted to HTTP_* variables
 * - Request body information for POST requests
 *
 * @param request The HTTP request object containing headers and body.
 * @param script_path The full filesystem path to the CGI script.
 * @return std::unique_ptr<char*[]> Environment array suitable for execve().
 */
std::unique_ptr<char*[], void(*)(char**)> CgiHandler::setupEnvironment(const HttpRequest& request, const std::string& script_path) {
    std::map<std::string, std::string> env;
    
    // Basic CGI environment variables
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["REQUEST_METHOD"] = request.getMethod();
    env["SCRIPT_NAME"] = request.getRequestTarget();
    env["SCRIPT_FILENAME"] = script_path;
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "WebServ/1.0";
    env["REDIRECT_STATUS"] = "200";
    
    // Query string
    std::string uri = request.getRequestTarget();
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        env["QUERY_STRING"] = uri.substr(query_pos + 1);
    } else {
        env["QUERY_STRING"] = "";
    }
    
    // Server info
    if (request.hasHeader("Host")) {
        std::string host = request.getHeader("Host");
        size_t colon_pos = host.find(':');
        if (colon_pos != std::string::npos) {
            env["SERVER_NAME"] = host.substr(0, colon_pos);
            env["SERVER_PORT"] = host.substr(colon_pos + 1);
        } else {
            env["SERVER_NAME"] = host;
            env["SERVER_PORT"] = "80";
        }
    }
    
    // Content info for POST
    if (request.getMethod() == "POST") {
        env["CONTENT_LENGTH"] = std::to_string(request.getBody().length());
        if (request.hasHeader("Content-Type")) {
            env["CONTENT_TYPE"] = request.getHeader("Content-Type");
        }
    }
    
    // HTTP headers - add common ones
    if (request.hasHeader("User-Agent")) {
        env["HTTP_USER_AGENT"] = request.getHeader("User-Agent");
    }
    if (request.hasHeader("Accept")) {
        env["HTTP_ACCEPT"] = request.getHeader("Accept");
    }
    
    // Additional CGI variables
    env["REMOTE_ADDR"] = "127.0.0.1";
    env["PATH_INFO"] = "";
    
    return mapToEnvArray(env);
}
