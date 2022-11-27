#pragma once

#include <unordered_map>

#include "Object.hpp"
#include "Token.hpp"

namespace lox::treewalk {
class Environment {
 public:
  Environment() = default;
  explicit Environment(const Ref<Environment>& enclosing);

  LoxObject& get(const Token& name);
  LoxObject& get_at(int distance, const Token& name);

  void assign(const Token& name, const LoxObject& value);
  void assign_at(int distance, const Token& name, const LoxObject& value);
  void define(const std::string& name, const LoxObject& value);

  [[nodiscard]] const Ref<Environment>& enclosing() const { return enclosing_; }

 private:
  Environment& ancestor(int distance);

  Ref<Environment> enclosing_;
  std::unordered_map<std::string, LoxObject> values_;
};
}  // namespace lox::treewalk
