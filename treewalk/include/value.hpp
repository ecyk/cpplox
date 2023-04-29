#pragma once

#include <unordered_map>
#include <variant>

#include "common.hpp"

#define IS_BOOL(value) (std::holds_alternative<bool>(value))
#define IS_NIL(value) (std::holds_alternative<std::monostate>(value))
#define IS_NUMBER(value) (std::holds_alternative<double>(value))
#define IS_CLASS(value) (std::holds_alternative<std::shared_ptr<Class>>(value))
#define IS_FUNCTION(value) \
  (std::holds_alternative<std::shared_ptr<Function>>(value))
#define IS_INSTANCE(value) \
  (std::holds_alternative<std::shared_ptr<Instance>>(value))
#define IS_NATIVE(value) \
  (std::holds_alternative<std::shared_ptr<Native>>(value))
#define IS_STRING(value) (std::holds_alternative<std::string>(value))

#define AS_BOOL(value) (std::get<bool>(value))
#define AS_NUMBER(value) (std::get<double>(value))
#define AS_CLASS(value) (std::get<std::shared_ptr<Class>>(value))
#define AS_FUNCTION(value) (std::get<std::shared_ptr<Function>>(value))
#define AS_INSTANCE(value) (std::get<std::shared_ptr<Instance>>(value))
#define AS_NATIVE(value) (std::get<std::shared_ptr<Native>>(value))
#define AS_STRING(value) (std::get<std::string>(value))

namespace lox::treewalk {
namespace stmt {
struct Function;
struct Class;
}  // namespace stmt

struct Function;
struct Native;
struct Class;
struct Instance;

using Value = std::variant<std::monostate, std::string, double, bool,
                           std::shared_ptr<Function>, std::shared_ptr<Native>,
                           std::shared_ptr<Class>, std::shared_ptr<Instance>>;

class Environment;
class Interpreter;

struct Function {
  Function(Environment* closure, stmt::Function* declaration, int arity,
           bool is_initializer)
      : closure{closure},
        declaration{declaration},
        arity{arity},
        is_initializer{is_initializer} {}

  Environment* closure;
  stmt::Function* declaration;
  int arity;
  bool is_initializer;
};

using NativeFn = Value (*)(int arg_count, Value* args);

struct Native {
  Native(NativeFn function, int arity) : function{function}, arity{arity} {}

  NativeFn function;
  int arity;
};

using Methods =
    std::unordered_map<std::string, Function, Hash, std::equal_to<>>;

struct Class {
  Class(Methods methods, stmt::Class* declaration, Class* superclass, int arity)
      : methods{std::move(methods)},
        declaration{declaration},
        superclass{superclass},
        arity{arity} {}

  Methods methods;
  stmt::Class* declaration;
  Class* superclass;
  int arity;
};

using Fields = std::unordered_map<std::string, Value, Hash, std::equal_to<>>;

struct Instance {
  explicit Instance(Class* class_) : class_{class_} {}

  Fields fields;
  Class* class_;
};

void print_value(const Value& value);

static inline bool is_falsey(const Value& value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
}  // namespace lox::treewalk
