#include "token.hpp"

namespace lox::treewalk {
Token::Token(TokenType token_type, std::string lexeme, size_t line)
    : token_type_{token_type}, lexeme_{std::move(lexeme)}, line_{line} {}

std::string Token::to_string() {
  return std::to_string(static_cast<int>(token_type_)) + " " + lexeme_;
}
}  // namespace lox::treewalk
