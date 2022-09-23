#include "Object.hpp"

#include "LoxCallable.hpp"

namespace lox::treewalk {
std::string stringify(const Object& object) {
  if (is<Nil>(object)) {
    return "nil";
  }
  if (is<String>(object)) {
    return std::get<String>(object);
  }
  if (is<Number>(object)) {
    return std::to_string(std::get<Number>(object));
  }
  if (is<Boolean>(object)) {
    if (std::get<Boolean>(object)) {
      return "true";
    } else {
      return "false";
    }
  }
  if (is<Callable>(object)) {
    return std::get<Callable>(object)->to_string();
  }

  return "";
}
}  // namespace lox::treewalk
