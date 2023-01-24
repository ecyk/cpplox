#pragma once

#include <unordered_map>

#include "object.hpp"
#include "token.hpp"

namespace lox::treewalk {
class Environment {
 public:
  Environment() = default;
  explicit Environment(const Ref<Environment>& enclosing);

  Object& get(const Token& name);
  Object& get_at(int distance, const Token& name);

  void assign(const Token& name, const Object& value);
  void assign_at(int distance, const Token& name, const Object& value);
  void define(const std::string& name, const Object& value);

  [[nodiscard]] const Ref<Environment>& enclosing() const { return enclosing_; }

 private:
  Environment& ancestor(int distance);

  Ref<Environment> enclosing_;
  std::unordered_map<std::string, Object> values_;
};
}  // namespace lox::treewalk
