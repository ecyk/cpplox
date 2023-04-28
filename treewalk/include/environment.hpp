#pragma once

#include "scanner.hpp"
#include "value.hpp"

namespace lox::treewalk {
class Environment {
  using Values = std::unordered_map<std::string, Value, Hash, std::equal_to<>>;

 public:
  Environment() = default;
  explicit Environment(const std::shared_ptr<Environment>& enclosing);

  const Value& get(const Token& name);
  const Value& get_at(int distance, const Token& name);

  void assign(const Token& name, const Value& value);
  void assign_at(int distance, const Token& name, const Value& value);
  void define(std::string name, const Value& value);

  [[nodiscard]] const std::shared_ptr<Environment>& enclosing() const {
    return enclosing_;
  }
  [[nodiscard]] Values& get_values() { return values_; }

 private:
  Environment& ancestor(int distance);

  std::shared_ptr<Environment> enclosing_;
  Values values_;
};
}  // namespace lox::treewalk
