#include "compiler.hpp"

#include <iomanip>
#include <iostream>

#include "scanner.hpp"

namespace lox::bytecode {
void Compiler::compile(const std::string& source) {
  Scanner scanner{source};

  int line = -1;
  for (;;) {
    const Token token = scanner.scan_token();
    if (token.line_ != line) {
      std::cout << std::setfill(' ') << std::setw(4) << token.line_ << ' ';
      line = token.line_;
    } else {
      std::cout << "   | ";
    }
    std::cout << std::setfill(' ') << std::setw(2) << token.type_ << " '"
              << token.lexeme_ << "'\n";

    if (token.type_ == TOKEN_EOF) {
      break;
    }
  }
}
}  // namespace lox::bytecode
