#pragma once

#include <functional>
#include <variant>

#include "Common.hpp"

namespace lox::treewalk {
namespace stmt {
class Function;
class Class;
}  // namespace stmt

using LoxNil = std::monostate;
using LoxString = std::string;
using LoxNumber = double;
using LoxBoolean = bool;

struct LoxCallable;
struct LoxInstance;

using LoxObject = std::variant<LoxNil, LoxString, LoxNumber, LoxBoolean,
                               Ref<LoxCallable>, Ref<LoxInstance>>;

using Arguments = std::vector<LoxObject>;
using Callback = std::function<LoxObject(const Arguments&)>;

using Fields = std::unordered_map<std::string, LoxObject>;
using Methods = std::unordered_map<std::string, LoxCallable>;

class Environment;
class Interpreter;

struct LoxCallable {
  struct NativeFunction {};

  struct LoxFunction {
    LoxFunction(const Ref<Environment>& closure, stmt::Function* declaration,
                bool is_initializer)
        : closure_{closure},
          declaration_{declaration},
          is_initializer_{is_initializer} {}

    Ref<LoxCallable> bind(const LoxObject& instance,
                          Interpreter* interpreter) const;

    Ref<Environment> closure_;
    stmt::Function* declaration_;
    bool is_initializer_;
  };

  struct LoxClass {
    LoxClass(Methods methods, stmt::Class* declaration, LoxObject* superclass)
        : methods_{std::move(methods)},
          declaration_{declaration},
          superclass_{superclass} {}

    [[nodiscard]] LoxCallable* find_method(const std::string& name);

    Methods methods_;
    stmt::Class* declaration_;
    LoxObject* superclass_;
  };

  using Data = std::variant<NativeFunction, LoxFunction, LoxClass>;

  LoxCallable(Callback call, size_t arity);
  LoxCallable(size_t arity, const Ref<Environment>& closure,
              stmt::Function* declaration, bool is_initializer,
              Interpreter* interpreter);
  LoxCallable(Methods methods, stmt::Class* declaration, LoxObject* superclass,
              Interpreter* interpreter);

  Callback call_;
  size_t arity_;
  Data data_{};

  Interpreter* interpreter_{nullptr};
};

struct LoxInstance {
  LoxInstance(LoxCallable* class_) : class_{class_} {}

  Fields fields;
  LoxCallable* class_;
};

std::string stringify(const LoxObject& object);
}  // namespace lox::treewalk
