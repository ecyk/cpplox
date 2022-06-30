#include <iostream>

#include "Lox.hpp"

using namespace lox;

int main(int argc, char* argv[]) {  // NOLINT
  try {
    if (argc > 2) {
      std::cerr << "Usage: cpplox [script]";
      return EXIT_FAILURE;
    } else if (argc == 2) {
      treewalk::run_file(argv[1]);  // NOLINT
    } else {
      treewalk::run_prompt();
    }

    if (treewalk::s_had_error) {
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  return EXIT_FAILURE;
}
