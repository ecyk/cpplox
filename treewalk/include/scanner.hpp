#pragma once

#include <vector>

#include "token.hpp"

namespace lox::treewalk {
class Scanner {
 public:
  explicit Scanner(std::string source);

  std::vector<Token> scan_tokens();

 private:
  void scan_token();
  void add_token(TokenType type);

  void comment();
  void multiline_comment();
  void string();
  void number();
  void identifier();

  [[nodiscard]] char peek() const;
  [[nodiscard]] char peek_next() const;
  bool match(char expected);
  char advance();

  [[nodiscard]] bool is_at_end() const;

 private:
  std::string source_;

  std::vector<Token> tokens_;
  size_t start_{}, current_{};
  size_t line_{1};
};
}  // namespace lox::treewalk
