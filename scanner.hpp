#pragma once

#include <string>
#include <vector>

namespace lox {
enum TokenType {
  // Single-character tokens.

  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_SEMICOLON,
  TOKEN_SLASH,
  TOKEN_STAR,

  // One or two character tokens.

  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,

  // Literals.

  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,

  // Keywords.

  TOKEN_AND,
  TOKEN_CLASS,
  TOKEN_ELSE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_FUN,
  TOKEN_IF,
  TOKEN_NIL,
  TOKEN_OR,
  TOKEN_PRINT,
  TOKEN_RETURN,
  TOKEN_SUPER,
  TOKEN_THIS,
  TOKEN_TRUE,
  TOKEN_VAR,
  TOKEN_WHILE,

  TOKEN_ERROR,
  TOKEN_EOF,
  TOKEN_COUNT
};

struct Token {
  Token() = default;
  Token(TokenType type, std::string_view lexeme, int line)
      : type{type}, lexeme{lexeme}, line{line} {}

  TokenType type{TOKEN_ERROR};
  std::string_view lexeme{};
  int line{};
};

class Scanner {
 public:
  explicit Scanner(const std::string& source)
      : start_{source.c_str()}, current_{source.c_str()} {}

  Token scan_token();
  std::vector<Token> scan_tokens();

 private:
  [[nodiscard]] Token make_token(TokenType type) const {
    return {type, {start_, static_cast<size_t>(current_ - start_)}, line_};
  }

  [[nodiscard]] Token error_token(std::string_view message) const {
    return {TOKEN_ERROR, message, line_};
  }

  Token identifier();
  [[nodiscard]] TokenType identifier_type() const;
  Token number();
  Token string();

  [[nodiscard]] char peek() const { return *current_; }
  [[nodiscard]] char peek_next() const;
  bool match(char expected);
  char advance() { return *current_++; }

  [[nodiscard]] bool is_at_end() const { return *current_ == '\0'; }

  const char *start_{}, *current_{};
  int line_{1};
};
}  // namespace lox
