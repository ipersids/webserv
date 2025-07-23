#include "../../includes/config.hpp"

void parse(int argc, char **argv, Config &config)
{
    std::string config_path;

    if (argc == 2) {
        config_path = argv[1];
    } else {
        // Throw an exception that will be caught in main
        throw std::runtime_error("Usage: ./webserv [path_to_config_file]");
    }

    // This line modifies the original 'config' object from main
    config = ConfigParser::parse(config_path);
}


int main(int argc, char *argv[]) {
    // 1. Handle command-line arguments to get the config file path
    Config config;

    try {
        parse(argc, argv, config);
    } catch(std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
		return -1;
    }

    std::cout << config.buffer.str() << std::endl;

    return 0;
}