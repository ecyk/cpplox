#include "Object.hpp"

#include "Interpreter.hpp"
#include "RuntimeError.hpp"

namespace lox::treewalk {

Ref<LoxCallable> LoxCallable::LoxFunction::bind(
    const LoxObject& instance, Interpreter* interpreter) const {
  auto closure = std::make_shared<Environment>(closure_);
  closure->define("this", instance);
  return std::make_shared<LoxCallable>(declaration_->params_.size(), closure,
                                       declaration_, is_initializer_,
                                       interpreter);
}

LoxCallable* LoxCallable::LoxClass::find_method(const std::string& name) {
  if (auto it = methods_.find(name); it != methods_.end()) {
    return &it->second;
  }

  if (superclass_ != nullptr) {
    auto& callable = as<Ref<LoxCallable>>(*superclass_);
    return as<LoxCallable::LoxClass>(callable->data_).find_method(name);
  }

  return nullptr;
}

LoxCallable::LoxCallable(Callback call, size_t arity)
    : call_{std::move(call)}, arity_{arity} {}

LoxCallable::LoxCallable(size_t arity, const Ref<Environment>& closure,
                         stmt::Function* declaration, bool is_initializer,
                         Interpreter* interpreter)
    : call_{[this](const Arguments& arguments) {
        const auto& data = as<LoxFunction>(data_);

        auto environment = std::make_shared<Environment>(data.closure_);

        const stmt::Function& function = *data.declaration_;
        for (size_t i = 0; i < function.params_.size(); i++) {
          environment->define(function.params_[i].get_lexeme(), arguments[i]);
        }

        // try {
        interpreter_->execute_block(function.body_, environment);
        // } catch (const Return& return_value) {
        //   return return_value.value_;
        // }

        if (data.is_initializer_) {
          return data.closure_->get_at(0, Token{TokenType::THIS, "this", 0});
        }

        return LoxObject{};
      }},
      arity_{arity},
      data_{LoxFunction{closure, declaration, is_initializer}},
      interpreter_{interpreter} {}

LoxCallable::LoxCallable(Methods methods, stmt::Class* declaration,
                         LoxObject* superclass, Interpreter* interpreter)
    : call_{[this](const Arguments& arguments) {
        auto& data = as<LoxClass>(data_);

        auto instance = std::make_shared<LoxInstance>(this);

        if (auto* initializer = data.find_method("init");
            initializer != nullptr) {
          as<LoxFunction>(initializer->data_)
              .bind(instance, interpreter_)
              ->call_(arguments);
        }

        return instance;
      }},
      arity_{0},
      data_{LoxClass{std::move(methods), declaration, superclass}},
      interpreter_{interpreter} {
  auto& data = as<LoxClass>(data_);
  if (auto* initializer = data.find_method("init"); initializer != nullptr) {
    arity_ = initializer->arity_;
  }
}

std::string stringify(const LoxObject& object) {
  if (is<LoxNil>(object)) {
    return "nil";
  }
  if (is<LoxString>(object)) {
    return as<LoxString>(object);
  }
  if (is<LoxNumber>(object)) {
    std::string number = std::to_string(as<LoxNumber>(object));
    number.erase(number.find_last_not_of('0') + 1, std::string::npos);
    number.erase(number.find_last_not_of('.') + 1, std::string::npos);
    return number;
  }
  if (is<LoxBoolean>(object)) {
    return as<LoxBoolean>(object) ? "true" : "false";
  }
  if (is<Ref<LoxCallable>>(object)) {
    const auto& callable = as<Ref<LoxCallable>>(object);

    if (is<LoxCallable::NativeFunction>(callable->data_)) {
      return "<native fn>";
    }
    if (is<LoxCallable::LoxFunction>(callable->data_)) {
      return "<fn " +
             as<LoxCallable::LoxFunction>(callable->data_)
                 .declaration_->name_.get_lexeme() +
             ">";
    }
    if (is<LoxCallable::LoxClass>(callable->data_)) {
      return as<LoxCallable::LoxClass>(callable->data_)
          .declaration_->name_.get_lexeme();
    }
  }
  if (is<Ref<LoxInstance>>(object)) {
    return as<LoxCallable::LoxClass>(
               as<Ref<LoxInstance>>(object)->class_->data_)
               .declaration_->name_.get_lexeme() +
           " instance";
  }

  return "";
}
}  // namespace lox::treewalk
