#include "Parser.hpp"

#include "Lox.hpp"

namespace lox::treewalk {
Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

expr::Expr::Ptr Parser::parse() {
  try {
    expression();
    return std::move(expr_ptr_);
  } catch (const ParseError& e) {
    return {};
  }
}

void Parser::expression() { equality(); }

void Parser::equality() {
  comparison();

  while (match(TokenType::BANG_EQUAL) || match(TokenType::EQUAL_EQUAL)) {
    Token op = previous();
    auto left = std::move(expr_ptr_);
    comparison();
    auto right = std::move(expr_ptr_);
    expr_ptr_ =
        std::make_unique<expr::Binary>(std::move(left), op, std::move(right));
  }
}

void Parser::comparison() {
  term();

  while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
         match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
    Token op = previous();
    auto left = std::move(expr_ptr_);
    term();
    auto right = std::move(expr_ptr_);
    expr_ptr_ =
        std::make_unique<expr::Binary>(std::move(left), op, std::move(right));
  }
}

void Parser::term() {
  factor();

  while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
    Token op = previous();
    auto left = std::move(expr_ptr_);
    factor();
    auto right = std::move(expr_ptr_);
    expr_ptr_ =
        std::make_unique<expr::Binary>(std::move(left), op, std::move(right));
  }
}

void Parser::factor() {
  unary();

  while (match(TokenType::SLASH) || match(TokenType::STAR)) {
    Token op = previous();
    auto left = std::move(expr_ptr_);
    unary();
    auto right = std::move(expr_ptr_);
    expr_ptr_ =
        std::make_unique<expr::Binary>(std::move(left), op, std::move(right));
  }
}

void Parser::unary() {
  if (match(TokenType::BANG) || match(TokenType::MINUS)) {
    Token op = previous();
    unary();
    auto right = std::move(expr_ptr_);
    expr_ptr_ = std::make_unique<expr::Unary>(op, std::move(right));
    return;
  }

  return primary();
}

void Parser::primary() {
  if (match(TokenType::FALSE)) {
    expr_ptr_ = std::make_unique<expr::Literal>(Object{false});
    return;
  }
  if (match(TokenType::TRUE)) {
    expr_ptr_ = std::make_unique<expr::Literal>(Object{true});
    return;
  }
  if (match(TokenType::NIL)) {
    expr_ptr_ = std::make_unique<expr::Literal>(Object{});
    return;
  }

  if (match(TokenType::NUMBER)) {
    double value = std::strtod(previous().get_lexeme().c_str(), nullptr);
    expr_ptr_ = std::make_unique<expr::Literal>(Object{value});
    return;
  }
  if (match(TokenType::STRING)) {
    std::string value = previous().get_lexeme();

    // Trim string
    value.pop_back();
    value.erase(value.begin());
    expr_ptr_ = std::make_unique<expr::Literal>(Object{value});
    return;
  }

  if (match(TokenType::LEFT_PAREN)) {
    expression();
    auto expr = std::move(expr_ptr_);
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    expr_ptr_ = std::make_unique<expr::Grouping>(std::move(expr));
    return;
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
  if (isAtEnd()) {
    return false;
  }
  return peek().get_token_type() == type;
}

Token Parser::advance() {
  if (!isAtEnd()) {
    current_++;
  }
  return previous();
}

bool Parser::isAtEnd() { return peek().get_token_type() == TokenType::EOF_; }

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

  while (!isAtEnd()) {
    if (previous().get_token_type() == TokenType::SEMICOLON) {
      return;
    }

    switch (peek().get_token_type()) {
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
