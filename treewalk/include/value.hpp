#pragma once

#include <unordered_map>
#include <variant>

#include "common.hpp"

#define IS_BOOL(value) (std::holds_alternative<bool>(value))
#define IS_NIL(value) (std::holds_alternative<std::monostate>(value))
#define IS_NUMBER(value) (std::holds_alternative<double>(value))
#define IS_STRING(value) (std::holds_alternative<std::string>(value))
#define IS_OBJ(value) (std::holds_alternative<Obj*>(value))
#define IS_CLASS(value) (is_obj_type(value, Obj::Type::CLASS))
#define IS_FUNCTION(value) (is_obj_type(value, Obj::Type::FUNCTION))
#define IS_INSTANCE(value) (is_obj_type(value, Obj::Type::INSTANCE))
#define IS_NATIVE(value) (is_obj_type(value, Obj::Type::NATIVE))

#define AS_BOOL(value) (std::get<bool>(value))
#define AS_NUMBER(value) (std::get<double>(value))
#define AS_STRING(value) (std::get<std::string>(value))
#define AS_OBJ(value) (std::get<Obj*>(value))
#define AS_CLASS(value) (static_cast<ObjClass*>(AS_OBJ(value)))
#define AS_FUNCTION(value) (static_cast<ObjFunction*>(AS_OBJ(value)))
#define AS_INSTANCE(value) (static_cast<ObjInstance*>(AS_OBJ(value)))
#define AS_NATIVE(value) (static_cast<ObjNative*>(AS_OBJ(value)))

namespace lox::treewalk {
namespace stmt {
struct Function;
struct Class;
}  // namespace stmt

class Environment;

struct Obj {
  enum class Type { CLASS, FUNCTION, INSTANCE, NATIVE, ENVIRONMENT };

  virtual ~Obj() = default;

  explicit Obj(Type type) : type{type} {}

  Obj(const Obj&) = delete;
  Obj& operator=(const Obj&) = delete;
  Obj(Obj&&) = default;
  Obj& operator=(Obj&&) = default;

  Type type;
  bool is_marked{};
  Obj* next_object{};
};

using Value = std::variant<std::monostate, std::string, double, bool, Obj*>;

struct ObjFunction : Obj {
  ObjFunction(Environment* closure, stmt::Function* declaration, int arity,
              bool is_initializer)
      : Obj{Type::FUNCTION},
        closure{closure},
        declaration{declaration},
        arity{arity},
        is_initializer{is_initializer} {}

  Environment* closure;
  stmt::Function* declaration;
  int arity;
  bool is_initializer;
};

using NativeFn = Value (*)(int arg_count, Value* args);

struct ObjNative : Obj {
  ObjNative(NativeFn function, int arity)
      : Obj{Type::NATIVE}, function{function}, arity{arity} {}

  NativeFn function;
  int arity;
};

using Methods =
    std::unordered_map<std::string, ObjFunction, Hash, std::equal_to<>>;

struct ObjClass : Obj {
  ObjClass(Methods methods, stmt::Class* declaration, ObjClass* superclass,
           int arity)
      : Obj{Type::CLASS},
        methods{std::move(methods)},
        declaration{declaration},
        superclass{superclass},
        arity{arity} {}

  Methods methods;
  stmt::Class* declaration;
  ObjClass* superclass;
  int arity;
};

using Fields = std::unordered_map<std::string, Value, Hash, std::equal_to<>>;

struct ObjInstance : Obj {
  explicit ObjInstance(ObjClass* class_)
      : Obj{Type::INSTANCE}, class_{class_} {}

  Fields fields;
  ObjClass* class_;
};

void print_value(const Value& value);

inline bool is_falsey(const Value& value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

inline bool is_obj_type(Value value, Obj::Type type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
}  // namespace lox::treewalk
