#pragma once

#include <limits>
#include <unordered_map>
#include <variant>

#include "common.hpp"

namespace lox::treewalk {
namespace stmt {
class Function;
class Class;
}  // namespace stmt

using Nil = std::monostate;
using String = std::string;
using Number = double;
using Boolean = bool;

class Function;
class Class;
class Instance;

class Object {
  using Variant = std::variant<Nil, String, Number, Boolean, Ref<Function>,
                               Ref<Class>, Ref<Instance>>;

 public:
  Object() : variant_{Nil{}} {}
  explicit Object(Variant variant) : variant_{std::move(variant)} {}

  template <typename ObjectT>
  [[nodiscard]] constexpr bool is() const {
    if constexpr (std::is_same_v<ObjectT, Function> ||
                  std::is_same_v<ObjectT, Class> ||
                  std::is_same_v<ObjectT, Instance>) {
      return std::holds_alternative<Ref<ObjectT>>(variant_);
    } else {
      return std::holds_alternative<ObjectT>(variant_);
    }
  }

  template <typename ObjectT>
  [[nodiscard]] constexpr const ObjectT& as() const {
    return as_impl<const Object, ObjectT>(*this);
  }

  template <typename ObjectT>
  constexpr ObjectT& as() {
    return as_impl<Object, ObjectT>(*this);
  }

  Object call(const std::vector<Object>& arguments);
  [[nodiscard]] int arity() const;

  [[nodiscard]] std::string stringify() const;
  [[nodiscard]] bool is_truthy() const;

 private:
  template <typename T, typename ObjectT>
  static constexpr auto& as_impl(T& t) {
    if constexpr (std::is_same_v<ObjectT, Function> ||
                  std::is_same_v<ObjectT, Class> ||
                  std::is_same_v<ObjectT, Instance>) {
      return *std::get<Ref<ObjectT>>(t.variant_);
    } else {
      return std::get<ObjectT>(t.variant_);
    }
  }

  Variant variant_;

  friend bool operator==(const Object& left, const Object& right);
};

inline bool operator==(const Object& left, const Object& right) {
  if (left.is<Number>() && right.is<Number>()) {
    return std::abs(left.as<Number>() - right.as<Number>()) <=
           std::max(std::abs(left.as<Number>()), std::abs(right.as<Number>())) *
               std::numeric_limits<Number>::epsilon();
  }

  return left.variant_ == right.variant_;
}

class Environment;
class Interpreter;

class Function {
 public:
  virtual ~Function() = default;

  explicit Function(const Ref<Environment>& closure = {},
                    bool is_initializer = false,
                    stmt::Function* declaration = nullptr,
                    Interpreter* interpreter = nullptr);

  Function(const Function&) = delete;
  Function& operator=(const Function&) = delete;
  Function(Function&&) = default;
  Function& operator=(Function&&) = default;

  virtual Object call(const std::vector<Object>& arguments);
  [[nodiscard]] virtual int arity() const { return arity_; }

  [[nodiscard]] Ref<Function> bind(const Object& instance) const;

  [[nodiscard]] std::string to_string() const;

 private:
  Ref<Environment> closure_;
  int arity_{};
  bool is_initializer_;

  stmt::Function* declaration_;
  Interpreter* interpreter_;
};

class Class {
 public:
  using Methods = std::unordered_map<std::string, Function>;

  Class(Methods methods, Object* superclass, stmt::Class* declaration);

  Object call(const std::vector<Object>& arguments);
  [[nodiscard]] int arity() const { return arity_; }

  [[nodiscard]] const Function* find_method(const std::string& name) const;

  [[nodiscard]] std::string to_string() const;

 private:
  Methods methods_;
  Object* superclass_;
  int arity_{};

  stmt::Class* declaration_;
};

class Instance {
 public:
  using Fields = std::unordered_map<std::string, Object>;

  explicit Instance(Class* class_) : class_{class_} {}

  [[nodiscard]] const Object* get_field(const std::string& name) const;
  [[nodiscard]] const Function* get_method(const std::string& name) const;

  void set_field(const std::string& name, const Object& object);

  [[nodiscard]] std::string to_string() const;

 private:
  Fields fields_;
  Class* class_;
};
}  // namespace lox::treewalk
