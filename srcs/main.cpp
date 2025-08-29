#include "Connection.hpp"
#include "Webserver.hpp"

int main(int argc, char **argv) {
  if (argc > 2) {
    std::cerr << "Usage: ./webserv [path_to_config_file]" << std::endl;
    return 1;
  }

  std::string path = "tests/test-configs/test.conf";
  if (argc == 2) {
    path = argv[1];
  }

  try {
    Webserv webserv(path);
    webserv.run();
  } catch (const std::runtime_error &e) {
    std::cerr << "\n\n[RUNTIME ERROR] " << e.what() << "\n" << std::endl;
    Logger::shutdown();
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "\n\n[EXCEPTION ERROR] " << e.what() << "\n" << std::endl;
    Logger::shutdown();
    return 1;
  } catch (...) {
    std::cerr << "\n\n[UKNOWN ERROR]" << std::endl;
    Logger::shutdown();
    return 1;
  }
  return 0;
}
