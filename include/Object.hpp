#pragma once

#include <memory>
#include <string>
#include <variant>

namespace lox::treewalk {
class LoxCallable;

using Nil = std::monostate;
using String = std::string;
using Number = double;
using Boolean = bool;
using Callable = std::shared_ptr<LoxCallable>;

using Object = std::variant<Nil, String, Number, Boolean, Callable>;

template <class Type, class... Types>
constexpr bool is(const std::variant<Types...>& variant) {
  return std::holds_alternative<Type>(variant);
}

std::string stringify(const Object& object);
}  // namespace lox::treewalk
