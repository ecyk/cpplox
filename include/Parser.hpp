#pragma once

#include <vector>

#include "Expr.hpp"
#include "Stmt.hpp"

namespace lox::treewalk {
class ParseError : public std::exception {
 public:
};

class Parser {
 public:
  Parser(std::vector<Token> tokens);

  std::vector<stmt::Stmt::Ptr> parse();

 private:
  stmt::Stmt::Ptr declaration();
  stmt::Stmt::Ptr var_declaration();
  stmt::Stmt::Ptr statement();
  stmt::Stmt::Ptr for_statement();
  stmt::Stmt::Ptr if_statement();
  stmt::Stmt::Ptr print_statement();
  stmt::Stmt::Ptr while_statement();
  stmt::Stmt::Ptr block_statement();
  stmt::Stmt::Ptr expression_statement();

  std::vector<stmt::Stmt::Ptr> block();

  expr::Expr::Ptr expression();
  expr::Expr::Ptr assignment();
  expr::Expr::Ptr logic_or();
  expr::Expr::Ptr logic_and();
  expr::Expr::Ptr equality();
  expr::Expr::Ptr comparison();
  expr::Expr::Ptr term();
  expr::Expr::Ptr factor();
  expr::Expr::Ptr unary();
  expr::Expr::Ptr primary();

  bool match(TokenType type);
  bool check(TokenType type);
  Token& advance();
  bool is_at_end();
  Token& peek();
  Token& previous();

  Token& consume(TokenType type, const std::string& message);

  static ParseError error(const Token& token, const std::string& message);

  void synchronize();

 private:
  std::vector<Token> tokens_;
  size_t current_ = 0;
};
}  // namespace lox::treewalk
