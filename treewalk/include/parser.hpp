#pragma once

#include "stmt.hpp"

namespace lox::treewalk {
struct ParseError : std::exception {};

class Parser {
 public:
  explicit Parser(std::vector<lox::Token> tokens);

  std::vector<std::unique_ptr<Stmt>> parse();

 private:
  std::unique_ptr<Stmt> declaration();
  std::unique_ptr<Stmt> class_declaration();
  stmt::Function fun_declaration(const std::string& kind);
  std::unique_ptr<Stmt> fun_declaration();
  std::unique_ptr<Stmt> var_declaration();
  std::unique_ptr<Stmt> statement();
  std::unique_ptr<Stmt> for_statement();
  std::unique_ptr<Stmt> if_statement();
  std::unique_ptr<Stmt> print_statement();
  std::unique_ptr<Stmt> return_statement();
  std::unique_ptr<Stmt> while_statement();
  std::unique_ptr<Stmt> block_statement();
  std::unique_ptr<Stmt> expression_statement();

  std::vector<std::unique_ptr<Stmt>> block();
  std::unique_ptr<Expr> finish_call(std::unique_ptr<Expr> callee);

  std::unique_ptr<Expr> expression();
  std::unique_ptr<Expr> assignment();
  std::unique_ptr<Expr> logic_or();
  std::unique_ptr<Expr> logic_and();
  std::unique_ptr<Expr> equality();
  std::unique_ptr<Expr> comparison();
  std::unique_ptr<Expr> term();
  std::unique_ptr<Expr> factor();
  std::unique_ptr<Expr> unary();
  std::unique_ptr<Expr> call();
  std::unique_ptr<Expr> primary();

  bool match(TokenType type);
  bool check(TokenType type);
  lox::Token& advance();
  bool is_at_end();
  lox::Token& peek();
  lox::Token& previous();

  lox::Token& consume(TokenType type, const std::string& message);

  static ParseError error(const lox::Token& token, const std::string& message);

  void synchronize();

  std::vector<lox::Token> tokens_;
  size_t current_{};
};
}  // namespace lox::treewalk
