#pragma once

#include <stdexcept>
#include <utility>

#include "Token.hpp"

namespace lox::treewalk {
class RuntimeError : public std::runtime_error {
 public:
  RuntimeError(const Token& token, const std::string& message)
      : token_{token}, runtime_error{message} {}

 public:
  Token token_;
};
}  // namespace lox::treewalk
