#pragma once

#include <stdexcept>
#include <utility>

#include "scanner.hpp"

namespace lox::treewalk {
struct RuntimeError : std::runtime_error {
  RuntimeError(Token token, const std::string& message)
      : runtime_error{message}, token{std::move(token)} {}

  Token token;
};
}  // namespace lox::treewalk
