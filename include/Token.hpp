#pragma once

#include <any>
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
  Token(TokenType token_type, std::string lexeme, std::any literal,
        size_t line);

  std::string to_string();

 private:
  TokenType token_type_;
  std::string lexeme_;
  std::any literal_;
  size_t line_;
};
}  // namespace lox::treewalk
