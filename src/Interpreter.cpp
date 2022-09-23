#include "Interpreter.hpp"

#include <chrono>
#include <iostream>

#include "Lox.hpp"
#include "LoxFunction.hpp"

namespace lox::treewalk {
class ClockFn : public LoxCallable {
 public:
  [[nodiscard]] size_t arity() const override { return 0; }

  void call(Interpreter& interpreter,
            const std::vector<Object>& /*arguments*/) override {
    interpreter.value_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count() /
        1000.0;
  }

  [[nodiscard]] std::string to_string() const override { return "<native fn>"; }
};

bool number_compare(Number a, Number b) {
  return std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) *
                                std::numeric_limits<Number>::epsilon();
}

Interpreter::Interpreter()
    : environment_{std::make_unique<Environment>(&globals_)} {
  globals_.define("clock", std::make_shared<ClockFn>());
}

void Interpreter::interpret(const std::vector<stmt::Stmt::Ptr>& statements) {
  try {
    for (const auto& statement : statements) {
      execute(statement);
    }
  } catch (const RuntimeError& e) {
    runtime_error(e);
  }
}

void Interpreter::execute_block(const std::vector<stmt::Stmt::Ptr>& statements,
                                std::unique_ptr<Environment> environment) {
  auto previous = std::move(environment_);

  // try {
  environment_ = std::move(environment);

  for (const auto& statement : statements) {
    execute(statement);
  }
  // } catch (...) {
  //   environment_ = previous;
  //   throw;
  // }

  environment_ = std::move(previous);
}

void Interpreter::execute(const stmt::Stmt::Ptr& stmt) {
  if (is_returning_) {
    return;
  }

  stmt->accept(*this);
}

void Interpreter::visit(stmt::Block& block) {
  execute_block(block.statements_,
                std::make_unique<Environment>(environment_.get()));
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(expression.expr_);
}

void Interpreter::visit(stmt::Function& function) {
  auto name = function.name_.get_lexeme();
  environment_->define(name, std::make_shared<LoxFunction>(std::move(function),
                                                           environment_.get()));
}

void Interpreter::visit(stmt::If& if_) {
  evaluate(if_.condition_);
  if (is_truthy(value_)) {
    execute(if_.then_branch_);
  } else if (if_.else_branch_) {
    execute(if_.else_branch_);
  }
}

void Interpreter::visit(stmt::Print& print) {
  evaluate(print.expr_);
  std::cout << stringify(value_) << '\n';
}

void Interpreter::visit(stmt::Return& return_) {
  if (return_.value_) {
    evaluate(return_.value_);
  }

  is_returning_ = true;
  // throw Return(value);
}

void Interpreter::visit(stmt::Var& var) {
  if (var.initializer_) {
    evaluate(var.initializer_);
  } else {
    value_ = {};
  }

  environment_->define(var.name_.get_lexeme(), std::move(value_));
}

void Interpreter::visit(stmt::While& while_) {
  while (true) {
    evaluate(while_.condition_);
    if (!is_truthy(value_)) {
      return;
    }
    execute(while_.body_);
  }
}

void Interpreter::evaluate(const expr::Expr::Ptr& expr) { expr->accept(*this); }

void Interpreter::visit(expr::Assign& assign) {
  evaluate(assign.value_);
  environment_->assign(assign.name_, std::move(value_));
}

void Interpreter::visit(expr::Binary& binary) {
  evaluate(binary.left_);
  auto left = std::move(value_);

  evaluate(binary.right_);
  auto right = std::move(value_);

  switch (binary.op_.get_type()) {
    case TokenType::BANG_EQUAL:
      value_ = !number_compare(std::get<Number>(left), std::get<Number>(right));
      break;
    case TokenType::EQUAL_EQUAL:
      value_ = number_compare(std::get<Number>(left), std::get<Number>(right));
      break;
    case TokenType::GREATER:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) > std::get<Number>(right);
      break;
    case TokenType::GREATER_EQUAL:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) >= std::get<Number>(right);
      break;
    case TokenType::LESS:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) < std::get<Number>(right);
      break;
    case TokenType::LESS_EQUAL:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) <= std::get<Number>(right);
      break;
    case TokenType::MINUS:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) - std::get<Number>(right);
      break;
    case TokenType::PLUS:
      if (is<Number>(left) && is<Number>(right)) {
        value_ = std::get<Number>(left) + std::get<Number>(right);
        break;
      }
      if (is<String>(left) && is<String>(right)) {
        value_ = std::get<String>(left) + std::get<String>(right);
        break;
      }

      throw RuntimeError(binary.op_,
                         "Operands must be two numbers or two strings.");
      break;
    case TokenType::SLASH:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) / std::get<Number>(right);
      break;
    case TokenType::STAR:
      check_number_operands(binary.op_, left, right);
      value_ = std::get<Number>(left) * std::get<Number>(right);
      break;
    default:
      break;
  }
}

void Interpreter::visit(expr::Call& call) {
  evaluate(call.callee_);
  auto callee = std::move(value_);

  std::vector<Object> arguments;
  for (const auto& argument : call.arguments_) {
    evaluate(argument);
    arguments.push_back(std::move(value_));
  }

  if (!is<Callable>(callee)) {
    throw RuntimeError(call.paren_, "Can only call functions and classes.");
  }

  auto& function = std::get<Callable>(callee);
  if (arguments.size() != function->arity()) {
    throw RuntimeError(call.paren_, "Expected " +
                                        std::to_string(function->arity()) +
                                        " arguments but got " +
                                        std::to_string(arguments.size()) + ".");
  }

  function->call(*this, arguments);
}

void Interpreter::visit(expr::Grouping& grouping) { evaluate(grouping.expr_); }

void Interpreter::visit(expr::Literal& literal) { value_ = literal.value_; }

void Interpreter::visit(expr::Logical& logical) {
  evaluate(logical.left_);

  if (logical.op_.get_type() == TokenType::OR) {
    if (is_truthy(value_)) {
      return;
    }
  } else {
    if (!is_truthy(value_)) {
      return;
    }
  }

  evaluate(logical.right_);
}

void Interpreter::visit(expr::Unary& unary) {
  evaluate(unary.right_);
  auto right = std::move(value_);

  switch (unary.op_.get_type()) {
    case TokenType::BANG:
      value_ = !is_truthy(right);
      break;
    case TokenType::MINUS:
      check_number_operand(unary.op_, right);
      value_ = -std::get<Number>(right);
      break;
    default:
      break;
  }
}

void Interpreter::visit(expr::Variable& variable) {
  value_ = environment_->get(variable.name_);
}

void Interpreter::check_number_operand(const Token& op, const Object& operand) {
  if (is<Number>(operand)) {
    return;
  }

  throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token& op, const Object& left,
                                        const Object& right) {
  if (is<Number>(left) && is<Number>(right)) {
    return;
  }

  throw RuntimeError(op, "Operands must be numbers.");
}

bool Interpreter::is_truthy(Object& value) {
  if (is<Nil>(value)) {
    return false;
  }
  if (is<Boolean>(value)) {
    return std::get<Boolean>(value);
  }

  return true;
}
}  // namespace lox::treewalk
