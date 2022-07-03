#include "Environment.hpp"

#include "RuntimeError.hpp"

namespace lox::treewalk {
Environment::Environment(std::shared_ptr<Environment> enclosing)
    : enclosing_{std::move(enclosing)} {}

Object& Environment::get(const Token& name) {
  if (auto it = values_.find(name.get_lexeme()); it != values_.end()) {
    return it->second;
  }

  if (enclosing_) {
    return enclosing_->get(name);
  }

  throw RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::assign(const Token& name, const Object& value) {
  if (auto it = values_.find(name.get_lexeme()); it != values_.end()) {
    it->second = value;
    return;
  }

  if (enclosing_) {
    return enclosing_->assign(name, value);
  }

  throw RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::define(const std::string& name, const Object& value) {
  values_.insert_or_assign(name, value);
}
}  // namespace lox::treewalk
