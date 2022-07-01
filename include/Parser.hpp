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
  expr::Expr::Ptr expression();
  expr::Expr::Ptr equality();
  expr::Expr::Ptr comparison();
  expr::Expr::Ptr term();
  expr::Expr::Ptr factor();
  expr::Expr::Ptr unary();
  expr::Expr::Ptr primary();

  bool match(TokenType type);
  bool check(TokenType type);
  Token advance();
  bool is_at_end();
  Token peek();
  Token previous();

  Token consume(TokenType type, const std::string& message);

  static ParseError error(const Token& token, const std::string& message);

  void synchronize();

 private:
  std::vector<Token> tokens_;
  size_t current_ = 0;
};
}  // namespace lox::treewalk
