#include <iostream>

#include "bytecode.hpp"
#include "treewalk.hpp"

using namespace lox;

int main(int argc, char* argv[]) {
  int exit_code = 0;

  try {
    if (argc == 3 && strcmp(argv[1], "treewalk") == 0) {
      exit_code = treewalk::run_file(argv[2]);
    } else if (argc == 2) {
      if (strcmp(argv[1], "treewalk") == 0) {
        treewalk::run_prompt();
      } else {
        exit_code = bytecode::run_file(argv[1]);
      }
    } else if (argc == 1) {
      bytecode::run_prompt();
    } else {
      std::cerr << "Usage: cpplox [treewalk] [script]";
      exit_code = 64;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    exit_code = 1;
  }

  return exit_code;
}
