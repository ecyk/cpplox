#include "object.hpp"

#include <utility>

#include "interpreter.hpp"
#include "runtime_error.hpp"

namespace lox::treewalk {
std::string Object::stringify() const {
  if (is<Nil>()) {
    return "nil";
  }
  if (is<String>()) {
    return as<String>();
  }
  if (is<Number>()) {
    std::string number = std::to_string(as<Number>());
    number.erase(number.find_last_not_of('0') + 1, std::string::npos);
    number.erase(number.find_last_not_of('.') + 1, std::string::npos);
    return number;
  }
  if (is<Boolean>()) {
    return as<Boolean>() ? "true" : "false";
  }
  if (is<Function>()) {
    return as<Function>().name();
  }
  if (is<Class>()) {
    return as<Class>().name();
  }
  if (is<Instance>()) {
    return as<Instance>().name();
  }

  return "";
}

bool Object::is_truthy() const {
  if (is<Nil>()) {
    return false;
  }
  if (is<Boolean>()) {
    return as<Boolean>();
  }

  return true;
}

Function::Function(const Ref<Environment>& closure, bool is_initializer,
                   stmt::Function* declaration, Interpreter* interpreter)
    : closure_{closure},
      is_initializer_{is_initializer},
      declaration_{declaration},
      interpreter_{interpreter} {}

Object Function::call(const std::vector<Object>& arguments) {
  auto environment = std::make_shared<Environment>(closure_);

  for (size_t i = 0; i < declaration_->params_.size(); i++) {
    environment->define(declaration_->params_[i].get_lexeme(), arguments[i]);
  }

  // try {
  interpreter_->execute_block(declaration_->body_, environment);
  // } catch (const Return& return_value) {
  //   return return_value.value_;
  // }

  if (is_initializer_) {
    return closure_->get_at(0, Token{TokenType::THIS, "this", 0});
  }

  return {};
}

int Function::arity() const {
  return (declaration_ != nullptr)
             ? static_cast<int>(declaration_->params_.size())
             : 0;
}

std::string Function::name() const {
  return (declaration_ != nullptr)
             ? "<fn " + declaration_->name_.get_lexeme() + ">"
             : "<native fn>";
}

Ref<Function> Function::bind(const Object& instance,
                             Interpreter* interpreter) const {
  auto closure = std::make_shared<Environment>(closure_);
  closure->define("this", instance);
  return std::make_shared<Function>(closure, is_initializer_, declaration_,
                                    interpreter);
}

Class::Class(Methods methods, Object* superclass, stmt::Class* declaration,
             Interpreter* interpreter)
    : methods_{std::move(methods)},
      superclass_{superclass},
      declaration_{declaration},
      interpreter_{interpreter} {}

Object Class::call(const std::vector<Object>& arguments) {
  Object instance{std::make_shared<Instance>(this)};

  if (const Function* initializer = find_method("init");
      initializer != nullptr) {
    initializer->bind(instance, interpreter_)->call(arguments);
  }

  return instance;
}

int Class::arity() const {
  const Function* initializer = find_method("init");
  return (initializer != nullptr) ? initializer->arity() : 0;
}

const std::string& Class::name() const {
  return declaration_->name_.get_lexeme();
}

const Function* Class::find_method(const std::string& name) const {
  if (auto it = methods_.find(name); it != methods_.end()) {
    return &it->second;
  }

  if (superclass_ != nullptr) {
    return superclass_->as<Class>().find_method(name);
  }

  return nullptr;
}

std::string Instance::name() const { return class_->name() + " instance"; }
}  // namespace lox::treewalk
