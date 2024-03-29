#pragma once

#include "scanner.hpp"
#include "value.hpp"

namespace lox::treewalk {
class Environment : public Obj {
  using Values = std::unordered_map<std::string, Value>;

 public:
  explicit Environment(Environment* enclosing = nullptr);

  const Value& get(const Token& name);
  const Value& get_at(int distance, const Token& name);

  void assign(const Token& name, const Value& value);
  void assign_at(int distance, const Token& name, const Value& value);
  void define(const std::string& name, const Value& value);

  [[nodiscard]] Environment* get_enclosing() const { return enclosing_; }
  [[nodiscard]] Values& get_values() { return values_; }

 private:
  Environment& ancestor(int distance);

  Environment* enclosing_{};
  Values values_;
};
}  // namespace lox::treewalk
