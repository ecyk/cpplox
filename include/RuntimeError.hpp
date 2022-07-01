#pragma once

#include <stdexcept>
#include <utility>

#include "Token.hpp"

namespace lox::treewalk {
class RuntimeError : public std::runtime_error {
 public:
  RuntimeError(Token token, const std::string& message)
      : token_{std::move(token)}, runtime_error{message} {}

  Token token_;
};
}  // namespace lox::treewalk
