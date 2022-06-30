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
    if (source_line.empty()) {
      break;
    }

    run(source_line);
    s_had_error = false;
  }
}

void error(size_t line, const std::string& message) {
  report(line, "", message);
}

void report(size_t line, const std::string& where, const std::string& message) {
  std::cout << "[line " + std::to_string(line) + "] Error" + where + ": " +
                   message
            << '\n';
  s_had_error = true;
}
}  // namespace lox::treewalk
