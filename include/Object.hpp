#pragma once

#include <memory>
#include <string>
#include <variant>

namespace lox::treewalk {
class LoxCallable;
class Object {
 public:
  using Nil = std::monostate;
  using String = std::string;
  using Number = double;
  using Boolean = bool;
  using Callable = std::shared_ptr<LoxCallable>;

 public:
  Object() = default;
  explicit Object(String value);
  explicit Object(Number value);
  explicit Object(Boolean value);
  explicit Object(Callable value);

  bool operator==(const Object& other) const { return value_ == other.value_; }
  bool operator!=(const Object& other) const { return !(*this == other); }

  template <typename T>
  bool is() const {
    return std::holds_alternative<T>(value_);
  }

  template <typename T>
  T& get() {
    return std::get<T>(value_);
  }

  std::string stringify();

 private:
  std::variant<Nil, String, Number, Boolean, Callable> value_;
};
}  // namespace lox::treewalk
