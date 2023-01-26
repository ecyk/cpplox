#include "object.hpp"

#include "interpreter.hpp"
#include "runtime_error.hpp"

namespace lox::treewalk {
Object Object::call(const std::vector<Object>& arguments) {
  if (is<Function>()) {
    return as<Function>().call(arguments);
  }

  return as<Class>().call(arguments);
}

int Object::arity() const {
  if (is<Function>()) {
    return as<Function>().arity();
  }

  return as<Class>().arity();
}

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
    return as<Function>().to_string();
  }
  if (is<Class>()) {
    return as<Class>().to_string();
  }
  if (is<Instance>()) {
    return as<Instance>().to_string();
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
      interpreter_{interpreter} {
  if (declaration_ != nullptr) {
    arity_ = static_cast<int>(declaration_->params_.size());
  }
}

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

Ref<Function> Function::bind(const Object& instance) const {
  auto closure = std::make_shared<Environment>(closure_);
  closure->define("this", instance);
  return std::make_shared<Function>(closure, is_initializer_, declaration_,
                                    interpreter_);
}

std::string Function::to_string() const {
  if (declaration_ != nullptr) {
    return "<fn " + declaration_->name_.get_lexeme() + ">";
  }

  return "<native fn>";
}

Class::Class(Methods methods, Object* superclass, stmt::Class* declaration)
    : methods_{std::move(methods)},
      superclass_{superclass},
      declaration_{declaration} {
  if (const Function* initializer = find_method("init")) {
    arity_ = initializer->arity();
  }
}

Object Class::call(const std::vector<Object>& arguments) {
  Object instance{std::make_shared<Instance>(this)};

  if (const Function* initializer = find_method("init")) {
    initializer->bind(instance)->call(arguments);
  }

  return instance;
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

std::string Class::to_string() const {
  return declaration_->name_.get_lexeme();
}

const Object* Instance::get_field(const std::string& name) const {
  if (auto it = fields_.find(name); it != fields_.end()) {
    return &it->second;
  }

  return nullptr;
}

const Function* Instance::get_method(const std::string& name) const {
  if (const Function* method = class_->find_method(name)) {
    return method;
  }

  return nullptr;
}

void Instance::set_field(const std::string& name, const Object& object) {
  fields_.insert_or_assign(name, object);
}

std::string Instance::to_string() const {
  return class_->to_string() + " instance";
}
}  // namespace lox::treewalk
