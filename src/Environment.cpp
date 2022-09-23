#include "Environment.hpp"

#include "RuntimeError.hpp"

namespace lox::treewalk {
Environment::Environment(Environment* enclosing) : enclosing_{enclosing} {}

Object& Environment::get(const Token& name) {
  if (auto it = values_.find(name.get_lexeme()); it != values_.end()) {
    return it->second;
  }

  if (enclosing_ != nullptr) {
    return enclosing_->get(name);
  }

  throw RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::assign(const Token& name, Object value) {
  if (auto it = values_.find(name.get_lexeme()); it != values_.end()) {
    it->second = std::move(value);
    return;
  }

  if (enclosing_ != nullptr) {
    return enclosing_->assign(name, std::move(value));
  }

  throw RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::define(const std::string& name, Object value) {
  values_.insert_or_assign(name, std::move(value));
}
}  // namespace lox::treewalk
