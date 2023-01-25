#pragma once

#include "token.hpp"

namespace lox::bytecode {
class Scanner {
 public:
  Scanner(const std::string& source)
      : start_{source.c_str()}, current_{source.c_str()} {}

  Token scan_token();

 private:
  [[nodiscard]] Token make_token(TokenType type) const {
    return {type, {start_, static_cast<size_t>(current_ - start_)}, line_};
  }

  [[nodiscard]] Token error_token(std::string_view message) const {
    return {TOKEN_ERROR, message, line_};
  }

  Token identifier();
  TokenType identifier_type() const;
  Token number();
  Token string();

  char peek() const { return *current_; }
  char peek_next() const;
  bool match(char expected);
  char advance() { return *current_++; }

  bool is_at_end() const { return *current_ == '\0'; }

  const char *start_{}, *current_{};
  int line_{1};
};
}  // namespace lox::bytecode
