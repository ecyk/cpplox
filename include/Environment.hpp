#pragma once

#include <memory>
#include <unordered_map>

#include "Object.hpp"
#include "Token.hpp"

namespace lox::treewalk {
class Environment {
 public:
  Environment(Environment* enclosing);

  Object& get(const Token& name);
  Object& get_at(int distance, const Token& name);

  void assign(const Token& name, Object value);
  void assign_at(int distance, const Token& name, Object value);
  void define(const std::string& name, Object value);

 private:
  Environment& ancestor(int distance);

 private:
  Environment* enclosing_;

  std::unordered_map<std::string, Object> values_;
};
}  // namespace lox::treewalk
