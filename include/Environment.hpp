#pragma once

#include <memory>
#include <unordered_map>

#include "Object.hpp"
#include "Token.hpp"

namespace lox::treewalk {
class Environment {
 public:
  Environment() = default;
  Environment(std::shared_ptr<Environment> enclosing);

  Object& get(const Token& name);
  void assign(const Token& name, const Object& value);
  void define(const std::string& name, const Object& value);

 private:
  std::shared_ptr<Environment> enclosing_;

  std::unordered_map<std::string, Object> values_;
};
}  // namespace lox::treewalk
