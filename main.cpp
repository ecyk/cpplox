#include <iostream>

#include "treewalk.hpp"

using namespace lox;

int main(int argc, char* argv[]) {
  int exit_code = 0;

  try {
    if (argc > 2) {
      std::cerr << "Usage: cpplox [script]";
      exit_code = 64;
    } else if (argc == 2) {
      exit_code = treewalk::run_file(argv[1]);
    } else {
      treewalk::run_prompt();
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    exit_code = 1;
  }

  return exit_code;
}
