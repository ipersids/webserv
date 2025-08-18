#include <iostream>

void run_http_request_tests();
void run_http_request_parser_tests();
void run_http_response_tests();
void run_http_method_handler_tests();

int main() {
  try {
    run_http_request_tests();
    run_http_request_parser_tests();
    run_http_response_tests();
    run_http_method_handler_tests();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Test failed with unknown exception" << std::endl;
    return 1;
  }
}
