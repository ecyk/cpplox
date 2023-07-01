#include "treewalk.hpp"

#include <fstream>
#include <iostream>

// #include "ast_printer.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "scanner.hpp"

namespace lox::treewalk {
namespace {
bool g_had_error{};
bool g_had_runtime_error{};

void run(const std::string& source) {
  Scanner scanner{source};
  std::vector<lox::Token> tokens = scanner.scan_tokens();

  // for (auto& token : tokens) {
  //   std::cout << token.to_string() << '\n';
  // }

  Parser parser{std::move(tokens)};
  std::vector<std::unique_ptr<Stmt>> statements = parser.parse();

  if (g_had_error) {
    return;
  }

  // AstPrinter ast_printer;
  // std::cout << ast_printer.print(expr) << '\n';

  Resolver resolver;
  resolver.resolve(statements);

  if (g_had_error) {
    return;
  }

  g_interpreter.interpret(statements);
}
}  // namespace

int run_file(const std::string& path) {
  std::ifstream file_stream{path};
  file_stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  const std::string source{std::istreambuf_iterator<char>{file_stream},
                           std::istreambuf_iterator<char>{}};

  run(source);
  g_interpreter.free_environments();

  if (g_had_error) {
    return 65;
  }
  if (g_had_runtime_error) {
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
    g_had_error = false;
  }
  g_interpreter.free_environments();
}

void runtime_error(const RuntimeError& error) {
  std::cerr << std::string{error.what()} + "\n[line " +
                   std::to_string(error.token.line) + "]\n";
  g_had_runtime_error = true;
}

void error(const lox::Token& token, const std::string& message) {
  if (token.type == TOKEN_EOF) {
    report(token.line, " at end", message);
  } else {
    report(token.line, " at '" + std::string{token.lexeme} + "'", message);
  }
}

void error(int line, const std::string& message) { report(line, "", message); }

void report(int line, const std::string& where, const std::string& message) {
  std::cerr << "[line " + std::to_string(line) + "] Error" + where + ": " +
                   message
            << '\n';
  g_had_error = true;
}
}  // namespace lox::treewalk
