#include "Lox.hpp"

#include <fstream>
#include <iostream>

#include "Scanner.hpp"

namespace lox::treewalk {
static void run(const std::string& source) {
  Scanner scanner{source};
  std::vector<Token> tokens = scanner.scan_tokens();

  for (auto& token : tokens) {
    std::cout << token.to_string() << '\n';
  }
}

void run_file(const std::string& path) {
  std::ifstream file_stream{path};
  file_stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  std::string source{std::istreambuf_iterator<char>{file_stream},
                     std::istreambuf_iterator<char>{}};

  run(source);
}

void run_prompt() {
  std::string source_line;
  for (;;) {
    std::cout << "> ";

    std::getline(std::cin, source_line);
    run(source_line);
  }
}
}  // namespace lox::treewalk
