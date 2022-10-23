#include "Lox.hpp"

#include <fstream>
#include <iostream>

#include "AstPrinter.hpp"
#include "Interpreter.hpp"
#include "Parser.hpp"
#include "Resolver.hpp"
#include "Scanner.hpp"

namespace lox::treewalk {
static bool s_had_error{false};
static bool s_had_runtime_error{false};

static void run(const std::string& source) {
  Scanner scanner{source};
  std::vector<Token> tokens = scanner.scan_tokens();

  // for (auto& token : tokens) {
  //   std::cout << token.to_string() << '\n';
  // }

  Parser parser{std::move(tokens)};
  std::vector<Scope<Stmt>> statements = parser.parse();

  if (s_had_error) {
    return;
  }

  // AstPrinter ast_printer;
  // std::cout << ast_printer.print(expr) << '\n';

  Resolver resolver;
  resolver.resolve(statements);

  if (s_had_error) {
    return;
  }

  static Interpreter interpreter;
  interpreter.interpret(statements);
}

int run_file(const std::string& path) {
  std::ifstream file_stream{path};
  file_stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  std::string source{std::istreambuf_iterator<char>{file_stream},
                     std::istreambuf_iterator<char>{}};

  run(source);

  if (s_had_error) {
    return 65;
  }
  if (s_had_runtime_error) {
    return 70;
  }

  return 0;
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
  std::cerr << std::string{error.what()} + "\n[line " +
                   std::to_string(error.token_.get_line()) + "]\n";
  s_had_runtime_error = true;
}

void error(const Token& token, const std::string& message) {
  if (token.get_type() == TokenType::EOF_) {
    report(token.get_line(), " at end", message);
  } else {
    report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
  }
}

void error(size_t line, const std::string& message) {
  report(line, "", message);
}

void report(size_t line, const std::string& where, const std::string& message) {
  std::cerr << "[line " + std::to_string(line) + "] Error" + where + ": " +
                   message
            << '\n';
  s_had_error = true;
}
}  // namespace lox::treewalk
