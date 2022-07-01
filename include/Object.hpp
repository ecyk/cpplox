#pragma once

#include <string>
#include <variant>

namespace lox::treewalk {
class Object {
 public:
  using Nil = const void*;
  using String = std::string;
  using Number = double;
  using Boolean = bool;

 public:
  Object() : value_{nullptr} {}
  explicit Object(String value) : value_{std::move(value)} {}
  explicit Object(Number value) : value_{value} {}
  explicit Object(Boolean value) : value_{value} {}

  bool Object::operator==(const Object& other) const {
    return value_ == other.value_;
  }

  bool Object::operator!=(const Object& other) const {
    return !(*this == other);
  }

  template <typename T>
  bool is() const {
    return std::holds_alternative<T>(value_);
  }

  template <typename T>
  T& get() {
    return std::get<T>(value_);
  }

  std::string stringify() {
    if (is<Nil>()) {
      return "nil";
    } else if (is<String>()) {
      return get<String>();
    } else if (is<Number>()) {
      return std::to_string(get<Number>());
    } else if (is<Boolean>()) {
      if (get<Boolean>()) {
        return "true";
      } else {
        return "false";
      }
    }
    return "";
  }

 private:
  std::variant<Nil, String, Number, Boolean> value_;
};
}  // namespace lox::treewalk
