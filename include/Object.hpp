#pragma once

#include <functional>
#include <variant>

#include "Common.hpp"

namespace lox::treewalk {
using Nil = std::monostate;
using String = std::string;
using Number = double;
using Boolean = bool;

struct Callable;
struct Instance;

using Object = std::variant<Nil, String, Number, Boolean, Callable, Instance>;

using Arguments = std::vector<Object>;
using Callback = std::function<void(const Arguments&)>;

class Environment;

struct Callable {
  enum class Type { FunctionOrMethod, Class, Native };

  Callable(Callback callback, size_t arity, Type type,
           void* declaration = nullptr, Environment* closure = nullptr);

  Callback callback;
  size_t arity;

  Type type;

  void* declaration;
  Environment* closure;
};

bool operator==(const Callable& left, const Callable& right);

struct Instance {
  using Fields = std::unordered_map<std::string, Object>;
  using Methods = std::unordered_map<std::string, Callable>;

  Instance(std::string_view name, const Ref<Methods>& methods);

  std::string_view name;
  Ref<Methods> methods;
  Ref<Fields> fields;
};

bool operator==(const Instance& left, const Instance& right);

std::string stringify(const Object& object);
}  // namespace lox::treewalk
