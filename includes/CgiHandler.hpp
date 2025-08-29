#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "config.hpp"
#include "Logger.hpp"
#include "HttpUtils.hpp"
#include <array>

#define CGI_BUFSIZE 4096
#define CGI_TIMEOUT 30

class CgiHandler {
    public:
        CgiHandler() = delete;


    static HttpResponse execute(const HttpRequest& request, const ConfigParser::LocationConfig& location);
    static bool isCgiRequest(const std::string& uri, const ConfigParser::LocationConfig& location);
    static void test_cgi();


    private:
    static HttpResponse createErrorResponse(HttpUtils::HttpStatusCode status, const std::string& message);
    static std::string getInterpreter(const std::string& script_path, const ConfigParser::LocationConfig& location);
    static bool validateScript(const std::string& script_path);

    static std::unique_ptr<char*[], void(*)(char**)> setupEnvironment(const HttpRequest& request, const std::string& script_path);
    static HttpResponse parseOutput(const std::string& output);

};