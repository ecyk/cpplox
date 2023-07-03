#include "environment.hpp"

#include "runtime_error.hpp"

namespace lox::treewalk {
Environment::Environment(Environment* enclosing)
    : Obj{Type::ENVIRONMENT}, enclosing_{enclosing} {}

const Value& Environment::get(const Token& name) {
  if (auto it = values_.find(name.lexeme); it != values_.end()) {
    return it->second;
  }

  if (enclosing_ != nullptr) {
    return enclosing_->get(name);
  }

  throw RuntimeError{name,
                     "Undefined variable '" + std::string{name.lexeme} + "'."};
}

const Value& Environment::get_at(int distance, const Token& name) {
  return ancestor(distance).get(name);
}

void Environment::assign(const Token& name, const Value& value) {
  if (auto it = values_.find(name.lexeme); it != values_.end()) {
    it->second = value;
    return;
  }

  if (enclosing_ != nullptr) {
    return enclosing_->assign(name, value);
  }

  throw RuntimeError{name,
                     "Undefined variable '" + std::string{name.lexeme} + "'."};
}

void Environment::assign_at(int distance, const Token& name,
                            const Value& value) {
  ancestor(distance).assign(name, value);
}

void Environment::define(const std::string& name, const Value& value) {
  values_.insert_or_assign(name, value);
}

Environment& Environment::ancestor(int distance) {
  Environment* environment = this;
  for (int i = 0; i < distance; i++) {
    environment = environment->enclosing_;
  }

  return *environment;
}
}  // namespace lox::treewalk
