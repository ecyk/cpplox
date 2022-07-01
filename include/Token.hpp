#pragma once

#include <string>

namespace lox::treewalk {
enum class TokenType {
  // Single-character tokens

  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,

  // One or two character tokens

  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,

  // Literals

  IDENTIFIER,
  STRING,
  NUMBER,

  // Keywords

  AND,
  CLASS,
  ELSE,
  FALSE,
  FUN,
  FOR,
  IF,
  NIL,
  OR,
  PRINT,
  RETURN,
  SUPER,
  THIS,
  TRUE,
  VAR,
  WHILE,

  EOF_
};

class Token {
 public:
  Token(TokenType token_type, std::string lexeme, size_t line);

  TokenType get_type() const { return token_type_; }
  std::string get_lexeme() const { return lexeme_; }
  size_t get_line() const { return line_; }

  std::string to_string();

 private:
  TokenType token_type_;
  std::string lexeme_;
  size_t line_;
};
}  // namespace lox::treewalk
