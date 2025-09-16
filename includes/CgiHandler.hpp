#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "HttpUtils.hpp"
#include <array>

#define CGI_BUFSIZE 4096
#define CGI_TIMEOUT 30

class CgiHandler {
    public:
        CgiHandler() = delete;


    static HttpResponse execute(const HttpRequest& request, const ConfigParser::LocationConfig& location, const std::string& file_path);
    static bool isCgiRequest(const std::string& file_path, const ConfigParser::LocationConfig& location);
    
    private:
    static HttpResponse createErrorResponse(HttpUtils::HttpStatusCode status, const std::string& message);
    static std::string getInterpreter(const std::string& script_path, const ConfigParser::LocationConfig& location);
    static HttpResponse executeCgiScript(const HttpRequest& request, const std::string& script_path, 
                                       const std::string& interpreter, int input_pipe[2], int output_pipe[2]);
    static std::string readCgiOutput(int output_fd, pid_t pid);

    static std::unique_ptr<char*[], void(*)(char**)> setupEnvironment(const HttpRequest& request, const std::string& script_path);
    static HttpResponse parseOutput(const std::string& output);

};