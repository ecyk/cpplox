#include "Interpreter.hpp"

#include <chrono>
#include <iostream>

#include "Lox.hpp"

namespace lox::treewalk {
Interpreter::Interpreter()
    : environment_{std::make_shared<Environment>()},
      globals_{environment_.get()} {
  environment_->define(
      "clock",
      std::make_shared<LoxCallable>(
          [](const Arguments&) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

            constexpr double ms_to_seconds = 1.0 / 1000;
            return LoxObject{static_cast<double>(ms) * ms_to_seconds};
          },
          0));
}

void Interpreter::interpret(const std::vector<Scope<Stmt>>& statements) {
  try {
    for (const auto& statement : statements) {
      execute(statement);
    }
  } catch (const RuntimeError& e) {
    runtime_error(e);
  }
}

void Interpreter::execute_block(const std::vector<Scope<Stmt>>& statements,
                                const Ref<Environment>& environment) {
  if (statements.empty()) {
    return_value_ = {};
    return;
  }

  auto previous = std::move(environment_);

  // try {
  environment_ = environment;

  for (const auto& statement : statements) {
    if (is_returning_) {
      break;
    }
    execute(statement);
    // not sure
  }
  // } catch (...) {
  //   environment_ = previous;
  //   throw;
  // }

  environment_ = std::move(previous);
}

void Interpreter::execute(const Scope<Stmt>& stmt) {
  if (is_returning_) {
    return;
  }

  stmt->accept(*this);
}

void Interpreter::visit(stmt::Block& block) {
  execute_block(block.statements_, std::make_shared<Environment>(environment_));
}

void Interpreter::visit(stmt::Class& class_) {
  LoxObject* object = nullptr;
  if (class_.superclass_) {
    const auto& superclass = class_.superclass_.value();

    object = &look_up_variable(superclass.name_, superclass);

    if (!(is<Ref<LoxCallable>>(*object) &&
          is<LoxCallable::LoxClass>(as<Ref<LoxCallable>>(*object)->data_))) {
      throw RuntimeError(superclass.name_, "Superclass must be a class.");
    }
  }

  environment_->define(class_.name_.get_lexeme(), {});

  if (object != nullptr) {
    environment_ = std::make_shared<Environment>(environment_);
    environment_->define("super", *object);
  }

  Methods methods;
  for (auto& method : class_.methods_) {
    const auto& name = method.name_.get_lexeme();
    methods.insert_or_assign(
        name, LoxCallable{method.params_.size(), environment_, &method,
                          name == "init", this});
  }

  if (object != nullptr) {
    environment_ = environment_->enclosing();
  }

  environment_->assign(
      class_.name_,
      std::make_shared<LoxCallable>(std::move(methods), &class_, object, this));
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(expression.expr_);
}

void Interpreter::visit(stmt::Function& function) {
  environment_->define(
      function.name_.get_lexeme(),
      std::make_shared<LoxCallable>(function.params_.size(), environment_,
                                    &function, false, this));
}

void Interpreter::visit(stmt::If& if_) {
  if (is_truthy(evaluate(if_.condition_))) {
    execute(if_.then_branch_);
  } else if (if_.else_branch_) {
    execute(if_.else_branch_);
  }
}

void Interpreter::visit(stmt::Print& print) {
  std::cout << stringify(evaluate(print.expr_)) << '\n';
}

void Interpreter::visit(stmt::Return& return_) {
  is_returning_ = true;

  if (return_.value_) {
    evaluate(return_.value_);
  } else {
    return_value_ = {};
  }

  // throw Return(value);
}

void Interpreter::visit(stmt::Var& var) {
  if (var.initializer_) {
    evaluate(var.initializer_);
  } else {
    return_value_ = {};
  }

  environment_->define(var.name_.get_lexeme(), return_value_);
}

void Interpreter::visit(stmt::While& while_) {
  while (true) {
    if (!is_truthy(evaluate(while_.condition_))) {
      return;
    }
    execute(while_.body_);
    if (is_returning_) {
      break;
    }
  }
}

LoxObject& Interpreter::evaluate(const Scope<Expr>& expr) {
  expr->accept(*this);

  return return_value_;
}

void Interpreter::visit(expr::Assign& assign) {
  const auto& value = evaluate(assign.value_);

  int distance = assign.depth_;
  if (distance >= 0) {
    environment_->assign_at(distance, assign.name_, value);
  } else {
    globals_->assign(assign.name_, value);
  }
}

void Interpreter::visit(expr::Binary& binary) {
  auto left = evaluate(binary.left_);
  const auto& right = evaluate(binary.right_);

  switch (binary.op_.get_type()) {
    case TokenType::BANG_EQUAL:
      return_value_ = !is_equal(left, right);
      break;
    case TokenType::EQUAL_EQUAL:
      return_value_ = is_equal(left, right);
      break;
    case TokenType::GREATER:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) > as<LoxNumber>(right);
      break;
    case TokenType::GREATER_EQUAL:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) >= as<LoxNumber>(right);
      break;
    case TokenType::LESS:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) < as<LoxNumber>(right);
      break;
    case TokenType::LESS_EQUAL:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) <= as<LoxNumber>(right);
      break;
    case TokenType::MINUS:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) - as<LoxNumber>(right);
      break;
    case TokenType::PLUS:
      if (is<LoxNumber>(left) && is<LoxNumber>(right)) {
        return_value_ = as<LoxNumber>(left) + as<LoxNumber>(right);
        break;
      }
      if (is<LoxString>(left) && is<LoxString>(right)) {
        return_value_ = as<LoxString>(left) + as<LoxString>(right);
        break;
      }

      throw RuntimeError(binary.op_,
                         "Operands must be two numbers or two strings.");
      break;
    case TokenType::SLASH:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) / as<LoxNumber>(right);
      break;
    case TokenType::STAR:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<LoxNumber>(left) * as<LoxNumber>(right);
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Call& call) {
  auto callee = evaluate(call.callee_);

  std::vector<LoxObject> arguments;
  for (const auto& argument : call.arguments_) {
    arguments.push_back(evaluate(argument));
  }

  if (!is<Ref<LoxCallable>>(callee)) {
    throw RuntimeError(call.paren_, "Can only call functions and classes.");
  }

  const auto& callable = as<Ref<LoxCallable>>(callee);

  if (arguments.size() != callable->arity_) {
    throw RuntimeError(call.paren_, "Expected " +
                                        std::to_string(callable->arity_) +
                                        " arguments but got " +
                                        std::to_string(arguments.size()) + ".");
  }

  is_returning_ = false;
  const auto& value = callable->call_(arguments);
  if (!is<LoxNil>(value)) {
    return_value_ = value;
  }
  is_returning_ = false;
}

void Interpreter::visit(expr::Get& get) {
  auto& object = evaluate(get.object_);
  if (is<Ref<LoxInstance>>(object)) {
    auto& instance = as<Ref<LoxInstance>>(object);

    if (auto it = instance->fields.find(get.name_.get_lexeme());
        it != instance->fields.end()) {
      return_value_ = it->second;
      return;
    }

    if (LoxCallable* method = as<LoxCallable::LoxClass>(instance->class_->data_)
                                  .find_method(get.name_.get_lexeme());
        method != nullptr) {
      return_value_ =
          as<LoxCallable::LoxFunction>(method->data_).bind(instance, this);
      return;
    }

    throw RuntimeError(get.name_,
                       "Undefined property '" + get.name_.get_lexeme() + "'.");
  }

  throw RuntimeError(get.name_, "Only instances have properties.");
}

void Interpreter::visit(expr::Grouping& grouping) { evaluate(grouping.expr_); }

void Interpreter::visit(expr::Literal& literal) {
  return_value_ = literal.value_;
}

void Interpreter::visit(expr::Logical& logical) {
  const auto& left = evaluate(logical.left_);

  if (logical.op_.get_type() == TokenType::OR) {
    if (is_truthy(left)) {
      return;
    }
  } else {
    if (!is_truthy(left)) {
      return;
    }
  }

  evaluate(logical.right_);
}

void Interpreter::visit(expr::Set& set) {
  auto object = evaluate(set.object_);

  if (!is<Ref<LoxInstance>>(object)) {
    throw RuntimeError(set.name_, "Only instances have fields.");
  }

  const auto& value = evaluate(set.value_);
  const auto& instance = as<Ref<LoxInstance>>(object);
  instance->fields.insert_or_assign(set.name_.get_lexeme(), value);
}

void Interpreter::visit(expr::This& this_) {
  return_value_ = look_up_variable(this_.keyword_, this_);
}

void Interpreter::visit(expr::Super& super) {
  int distance = super.depth_;

  const auto& superclass =
      environment_->get_at(distance, Token{TokenType::SUPER, "super", 0});
  const auto& object =
      environment_->get_at(distance - 1, Token{TokenType::THIS, "this", 0});

  const auto& callable = as<Ref<LoxCallable>>(superclass);
  const auto& method = as<LoxCallable::LoxClass>(callable->data_)
                           .find_method(super.method_.get_lexeme());

  if (method == nullptr) {
    throw RuntimeError(super.method_, "Undefined property '" +
                                          super.method_.get_lexeme() + "'.");
  }

  return_value_ =
      as<LoxCallable::LoxFunction>(method->data_).bind(object, this);
}

void Interpreter::visit(expr::Unary& unary) {
  const auto& right = evaluate(unary.right_);

  switch (unary.op_.get_type()) {
    case TokenType::BANG:
      return_value_ = !is_truthy(right);
      break;
    case TokenType::MINUS:
      check_number_operand(unary.op_, right);
      return_value_ = -as<LoxNumber>(right);
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Variable& variable) {
  return_value_ = look_up_variable(variable.name_, variable);
}

LoxObject& Interpreter::look_up_variable(const Token& name,
                                         const expr::Expr& expr) {
  int distance = expr.depth_;
  if (distance >= 0) {
    return environment_->get_at(distance, name);
  }

  return globals_->get(name);
}

void Interpreter::check_number_operand(const Token& op,
                                       const LoxObject& operand) {
  if (is<LoxNumber>(operand)) {
    return;
  }

  throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token& op, const LoxObject& left,
                                        const LoxObject& right) {
  if (is<LoxNumber>(left) && is<LoxNumber>(right)) {
    return;
  }

  throw RuntimeError(op, "Operands must be numbers.");
}

bool Interpreter::is_truthy(const LoxObject& value) {
  if (is<LoxNil>(value)) {
    return false;
  }
  if (is<LoxBoolean>(value)) {
    return as<LoxBoolean>(value);
  }

  return true;
}

bool Interpreter::is_equal(const LoxObject& left, const LoxObject& right) {
  if (is<LoxNumber>(left) && is<LoxNumber>(right)) {
    return is_equal(as<LoxNumber>(left), as<LoxNumber>(right));
  }

  return left == right;
}

bool Interpreter::is_equal(LoxNumber a, LoxNumber b) {
  return std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) *
                                std::numeric_limits<LoxNumber>::epsilon();
}
}  // namespace lox::treewalk
