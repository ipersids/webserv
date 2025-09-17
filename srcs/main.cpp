#include "Connection.hpp"
#include "Webserver.hpp"

volatile std::sig_atomic_t shutdown_requested = 0;

int main(int argc, char **argv) {
  signal(SIGINT, Webserv::set_exit_to_true);
  signal(SIGTERM, Webserv::set_exit_to_true);
  signal(SIGPIPE, SIG_IGN);  // ignores it, writing to a closed socket wont
                             // crash the program
  signal(SIGCHLD, SIG_IGN);  // Apparently this tells the kernel to reap
                             // children, so no zombies?

  if (argc > 2) {
    std::cerr << "Usage: ./webserv [path_to_config_file]" << std::endl;
    return 1;
  }

  std::string path = "tests/test-configs/test.conf";
  if (argc == 2) {
    path = argv[1];
    std::cout << "Using the config file from arguments: " << path << std::endl;
  }
  else
    std::cout << "Using the default config: ./tests/test-configs/test.conf" << std::endl;

  try {
    Webserv webserv(path);
    webserv.run();
  } catch (const std::runtime_error &e) {
    std::cerr << "\n\n[RUNTIME ERROR] " << e.what() << "\n" << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "\n\n[EXCEPTION ERROR] " << e.what() << "\n" << std::endl;

    return 1;
  } catch (...) {
    std::cerr << "\n\n[UKNOWN ERROR]" << std::endl;
    return 1;
  }
  if (shutdown_requested) return 128 + shutdown_requested;
  return 0;
}
