#pragma once

#include "Interpreter.hpp"
#include "LoxCallable.hpp"

namespace lox::treewalk {
class LoxFunction : public LoxCallable {
 public:
  explicit LoxFunction(stmt::Function declaration, Environment* closure)
      : declaration_{std::move(declaration)}, closure_{closure} {}

  size_t arity() const override { return declaration_.params_.size(); }

  void call(Interpreter& interpreter,
            const std::vector<Object>& arguments) override {
    auto environment = std::make_unique<Environment>(closure_);

    for (size_t i = 0; i < declaration_.params_.size(); i++) {
      environment->define(declaration_.params_[i].get_lexeme(), arguments[i]);
    }

    // try {
    interpreter.execute_block(declaration_.body_, std::move(environment));
    // } catch (const Return& return_value) {
    //   return return_value.value_;
    // }

    interpreter.is_returning_ = false;
  }

  std::string to_string() const override {
    return "<fn " + declaration_.name_.get_lexeme() + ">";
  }

 public:
  stmt::Function declaration_;
  Environment* closure_;
};
}  // namespace lox::treewalk
