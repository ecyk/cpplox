#include "Token.hpp"

namespace lox::treewalk {
Token::Token(TokenType token_type, std::string lexeme, size_t line)
    : token_type_{token_type}, lexeme_{std::move(lexeme)}, line_{line} {}

Token::Token(TokenType token_type, std::string lexeme, std::any literal,
             size_t line)
    : token_type_{token_type},
      lexeme_{std::move(lexeme)},
      literal_{std::move(literal)},
      line_{line} {}

std::string Token::to_string() {
  std::string string =
      std::to_string(static_cast<int>(token_type_)) + " " + lexeme_ + " ";

  switch (token_type_) {
    case TokenType::STRING:
      string += std::any_cast<std::string>(literal_);
      break;
    case TokenType::NUMBER:
      string += std::to_string(std::any_cast<double>(literal_));
      break;
    default:
      string += "";
      break;
  }

  return string;
}
}  // namespace lox::treewalk
