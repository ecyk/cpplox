#include "Parser.hpp"

#include "Lox.hpp"

namespace lox::treewalk {
Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

expr::Expr::Ptr Parser::parse() {
  try {
    return expression();
  } catch (const ParseError& e) {
    return {};
  }
}

expr::Expr::Ptr Parser::expression() { return equality(); }

expr::Expr::Ptr Parser::equality() {
  auto expr = comparison();

  while (match(TokenType::BANG_EQUAL) || match(TokenType::EQUAL_EQUAL)) {
    Token op = previous();
    auto right = comparison();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

expr::Expr::Ptr Parser::comparison() {
  auto expr = term();

  while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
         match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
    Token op = previous();
    auto right = term();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

expr::Expr::Ptr Parser::term() {
  auto expr = factor();

  while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
    Token op = previous();
    auto right = factor();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

expr::Expr::Ptr Parser::factor() {
  auto expr = unary();

  while (match(TokenType::SLASH) || match(TokenType::STAR)) {
    Token op = previous();
    auto right = unary();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

expr::Expr::Ptr Parser::unary() {
  if (match(TokenType::BANG) || match(TokenType::MINUS)) {
    Token op = previous();
    auto right = unary();
    return std::make_unique<expr::Unary>(op, std::move(right));
  }

  return primary();
}

expr::Expr::Ptr Parser::primary() {
  if (match(TokenType::FALSE)) {
    return std::make_unique<expr::Literal>(Object{false});
  }
  if (match(TokenType::TRUE)) {
    return std::make_unique<expr::Literal>(Object{true});
  }
  if (match(TokenType::NIL)) {
    return std::make_unique<expr::Literal>(Object{});
  }

  if (match(TokenType::NUMBER)) {
    double value = std::strtod(previous().get_lexeme().c_str(), nullptr);
    return std::make_unique<expr::Literal>(Object{value});
  }
  if (match(TokenType::STRING)) {
    std::string value = previous().get_lexeme();

    // Trim string
    value.pop_back();
    value.erase(value.begin());
    return std::make_unique<expr::Literal>(Object{value});
  }

  if (match(TokenType::LEFT_PAREN)) {
    auto expr = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return std::make_unique<expr::Grouping>(std::move(expr));
  }

  throw error(peek(), "Expect expression.");
}

bool Parser::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }

  return false;
}

bool Parser::check(TokenType type) {
  if (is_at_end()) {
    return false;
  }

  return peek().get_type() == type;
}

Token Parser::advance() {
  if (!is_at_end()) {
    current_++;
  }

  return previous();
}

bool Parser::is_at_end() { return peek().get_type() == TokenType::EOF_; }

Token Parser::peek() { return tokens_.at(current_); }

Token Parser::previous() { return tokens_.at(current_ - 1); }

Token Parser::consume(TokenType type, const std::string& message) {
  if (check(type)) {
    return advance();
  }

  throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
  treewalk::error(token, message);
  return {};
}

void Parser::synchronize() {
  advance();

  while (!is_at_end()) {
    if (previous().get_type() == TokenType::SEMICOLON) {
      return;
    }

    switch (peek().get_type()) {
      case TokenType::CLASS:
      case TokenType::FUN:
      case TokenType::VAR:
      case TokenType::FOR:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::PRINT:
      case TokenType::RETURN:
        return;
      default:
        break;
    }

    advance();
  }
}
}  // namespace lox::treewalk
