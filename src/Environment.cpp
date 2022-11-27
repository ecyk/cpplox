#include "Environment.hpp"

#include "RuntimeError.hpp"

namespace lox::treewalk {
Environment::Environment(const Ref<Environment>& enclosing)
    : enclosing_{enclosing} {}

LoxObject& Environment::get(const Token& name) {
  if (auto it = values_.find(name.get_lexeme()); it != values_.end()) {
    return it->second;
  }

  if (enclosing_ != nullptr) {
    return enclosing_->get(name);
  }

  throw RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

LoxObject& Environment::get_at(int distance, const Token& name) {
  return ancestor(distance).get(name);
}

void Environment::assign(const Token& name, const LoxObject& value) {
  if (auto it = values_.find(name.get_lexeme()); it != values_.end()) {
    it->second = value;
    return;
  }

  if (enclosing_ != nullptr) {
    return enclosing_->assign(name, value);
  }

  throw RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::assign_at(int distance, const Token& name,
                            const LoxObject& value) {
  ancestor(distance).assign(name, value);
}

void Environment::define(const std::string& name, const LoxObject& value) {
  values_.insert_or_assign(name, value);
}

Environment& Environment::ancestor(int distance) {
  Environment* environment = this;
  for (int i = 0; i < distance; i++) {
    environment = environment->enclosing_.get();
  }

  return *environment;
}
}  // namespace lox::treewalk
