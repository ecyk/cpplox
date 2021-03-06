#pragma once

#include <vector>

#include "Object.hpp"

namespace lox::treewalk {
class Interpreter;
class LoxCallable {
 public:
  virtual ~LoxCallable() = default;

  virtual size_t arity() = 0;
  virtual void call(Interpreter& interpreter,
                    const std::vector<Object>& arguments) = 0;

  virtual std::string to_string() = 0;
};
}  // namespace lox::treewalk
