#pragma once

#include <vector>

#include "Expr.hpp"

namespace lox::treewalk {
class ParseError : public std::exception {
 public:
};

class Parser {
 public:
  Parser(std::vector<Token> tokens);

  expr::Expr::Ptr parse();

 private:
  void expression();
  void equality();
  void comparison();
  void term();
  void factor();
  void unary();
  void primary();

  bool match(TokenType type);
  bool check(TokenType type);
  Token advance();
  bool isAtEnd();
  Token peek();
  Token previous();

  Token consume(TokenType type, const std::string& message);

  static ParseError error(const Token& token, const std::string& message);

  void synchronize();

 private:
  std::vector<Token> tokens_;
  size_t current_ = 0;

  expr::Expr::Ptr expr_ptr_;
};
}  // namespace lox::treewalk
