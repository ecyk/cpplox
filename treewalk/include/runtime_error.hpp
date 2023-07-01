#pragma once

#include <stdexcept>
#include <utility>

#include "scanner.hpp"

namespace lox::treewalk {
struct RuntimeError : std::runtime_error {
  RuntimeError(const Token& token, const std::string& message)
      : runtime_error{message}, token{token} {}

  Token token;
};
}  // namespace lox::treewalk
