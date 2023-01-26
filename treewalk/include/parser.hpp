#pragma once

#include "stmt.hpp"

namespace lox::treewalk {
class ParseError : public std::exception {
 public:
};

class Parser {
 public:
  Parser(std::vector<Token> tokens);

  std::vector<Scope<Stmt>> parse();

 private:
  Scope<Stmt> declaration();
  Scope<Stmt> class_declaration();
  stmt::Function fun_declaration(const std::string& kind);
  Scope<Stmt> fun_declaration();
  Scope<Stmt> var_declaration();
  Scope<Stmt> statement();
  Scope<Stmt> for_statement();
  Scope<Stmt> if_statement();
  Scope<Stmt> print_statement();
  Scope<Stmt> return_statement();
  Scope<Stmt> while_statement();
  Scope<Stmt> block_statement();
  Scope<Stmt> expression_statement();

  std::vector<Scope<Stmt>> block();
  Scope<Expr> finish_call(Scope<Expr> callee);

  Scope<Expr> expression();
  Scope<Expr> assignment();
  Scope<Expr> logic_or();
  Scope<Expr> logic_and();
  Scope<Expr> equality();
  Scope<Expr> comparison();
  Scope<Expr> term();
  Scope<Expr> factor();
  Scope<Expr> unary();
  Scope<Expr> call();
  Scope<Expr> primary();

  bool match(TokenType type);
  bool check(TokenType type);
  Token& advance();
  bool is_at_end();
  Token& peek();
  Token& previous();

  Token& consume(TokenType type, const std::string& message);

  static ParseError error(const Token& token, const std::string& message);

  void synchronize();

  std::vector<Token> tokens_;
  size_t current_{};
};
}  // namespace lox::treewalk
