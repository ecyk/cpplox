#include "Lox.hpp"

#include <fstream>
#include <iostream>

#include "AstPrinter.hpp"
#include "Interpreter.hpp"
#include "Parser.hpp"
#include "Scanner.hpp"

namespace lox::treewalk {
static void run(const std::string& source) {
  Scanner scanner{source};
  std::vector<Token> tokens = scanner.scan_tokens();

  for (auto& token : tokens) {
    std::cout << token.to_string() << '\n';
  }

  Parser parser{tokens};
  auto expr = parser.parse();

  if (s_had_error) {
    return;
  }

  AstPrinter ast_printer;
  std::cout << ast_printer.print(expr) << '\n';

  static Interpreter interpreter;
  interpreter.interpret(expr);
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

void runtime_error(const RuntimeError& error) {
  std::cout << std::string{error.what()} + "\n[line " +
                   std::to_string(error.token_.get_line()) + "]\n";
  s_had_runtime_error = true;
}

void error(const Token& token, const std::string& message) {
  if (token.get_token_type() == TokenType::EOF_) {
    report(token.get_line(), " at end", message);
  } else {
    report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
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
