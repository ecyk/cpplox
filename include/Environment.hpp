#pragma once

#include <map>
#include <memory>

#include "Object.hpp"
#include "Token.hpp"

namespace lox::treewalk {
class Environment {
 public:
  Environment() = default;
  Environment(Environment* enclosing);

  Object& get(const Token& name);
  void assign(const Token& name, const Object& value);
  void define(const std::string& name, Object value);

 private:
  Environment* enclosing_;

  std::map<std::string, Object> values_;
};
}  // namespace lox::treewalk
