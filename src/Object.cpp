#include "Object.hpp"

#include "LoxCallable.hpp"

namespace lox::treewalk {
Object::Object(String value) : value_{std::move(value)} {}
Object::Object(Number value) : value_{value} {}
Object::Object(Boolean value) : value_{value} {}
Object::Object(Callable value) : value_{std::move(value)} {}

std::string Object::stringify() {
  if (is<Nil>()) {
    return "nil";
  }
  if (is<String>()) {
    return get<String>();
  }
  if (is<Number>()) {
    return std::to_string(get<Number>());
  }
  if (is<Boolean>()) {
    if (get<Boolean>()) {
      return "true";
    } else {
      return "false";
    }
  }
  if (is<Callable>()) {
    return get<Callable>()->to_string();
  }

  return "";
}
}  // namespace lox::treewalk
