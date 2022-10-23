#include "Object.hpp"

#include "RuntimeError.hpp"
#include "Stmt.hpp"

namespace lox::treewalk {
Callable::Callable(Callback callback, size_t arity, Type type,
                   void* declaration, Environment* closure)
    : callback{std::move(callback)},
      arity{arity},
      type{type},
      declaration{declaration},
      closure{closure} {}

bool operator==(const Callable& left, const Callable& right) {
  return left.type == right.type && left.declaration == right.declaration &&
         left.closure == right.closure;
}

Instance::Instance(std::string_view name, const Ref<Methods>& methods)
    : name{name}, methods{methods}, fields{std::make_shared<Fields>()} {}

bool operator==(const Instance& left, const Instance& right) {
  return left.name == right.name && left.fields == right.fields;
}

std::string stringify(const Object& object) {
  if (is<Nil>(object)) {
    return "nil";
  }
  if (is<String>(object)) {
    return as<String>(object);
  }
  if (is<Number>(object)) {
    double value = as<Number>(object);

    std::string str = std::to_string(value);
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    str.erase(str.find_last_not_of('.') + 1, std::string::npos);
    return str;
  }
  if (is<Boolean>(object)) {
    bool value = as<Boolean>(object);
    return value ? "true" : "false";
  }
  if (is<Callable>(object)) {
    const auto& callable = as<Callable>(object);

    switch (callable.type) {
      case Callable::Type::FunctionOrMethod:
        return "<fn " +
               static_cast<stmt::Function*>(callable.declaration)
                   ->name_.get_lexeme() +
               ">";
      case Callable::Type::Class:
        return static_cast<stmt::Class*>(callable.declaration)
            ->name_.get_lexeme();
      case Callable::Type::Native:
        return "<native fn>";
      default:
        return "";
    }
  }
  if (is<Instance>(object)) {
    const auto& value = as<Instance>(object);
    return std::string{value.name} + " instance";
  }

  return "";
}
}  // namespace lox::treewalk
