#include "Interpreter.hpp"

#include <iostream>

#include "Lox.hpp"
#include "RuntimeError.hpp"

namespace lox::treewalk {
Interpreter::Interpreter() : environment_{std::make_shared<Environment>()} {}

void Interpreter::interpret(const std::vector<stmt::Stmt::Ptr>& statements) {
  try {
    for (const auto& statement : statements) {
      execute(*statement);
    }
  } catch (const RuntimeError& e) {
    runtime_error(e);
  }
}

void Interpreter::execute(stmt::Stmt& stmt) { stmt.accept(*this); }

void Interpreter::execute_block(const std::vector<stmt::Stmt::Ptr>& statements,
                                std::shared_ptr<Environment> environment) {
  auto previous = environment_;
  try {
    environment_ = std::move(environment);

    for (const auto& statement : statements) {
      execute(*statement);
    }

    environment_ = previous;
  } catch (const RuntimeError&) {
    environment_ = previous;
    throw;
  }
}

void Interpreter::visit(stmt::Block& block) {
  execute_block(block.statements_, std::make_shared<Environment>(environment_));
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(*expression.expr_);
}

void Interpreter::visit(stmt::If& if_) {
  evaluate(*if_.condition_);
  if (is_truthy(value_)) {
    execute(*if_.then_branch_);
  } else if (if_.else_branch_) {
    execute(*if_.else_branch_);
  }
}

void Interpreter::visit(stmt::Print& print) {
  evaluate(*print.expr_);
  std::cout << value_.stringify() << '\n';
}

void Interpreter::visit(stmt::Var& var) {
  auto value = Object{};
  if (var.initializer_ != nullptr) {
    evaluate(*var.initializer_);
    value = std::move(value_);
  }

  environment_->define(var.name_.get_lexeme(), value);
}

void Interpreter::visit(stmt::While& while_) {
  evaluate(*while_.condition_);
  while (is_truthy(value_)) {
    execute(*while_.body_);
    evaluate(*while_.condition_);
  }
}

void Interpreter::evaluate(expr::Expr& expr) { expr.accept(*this); }

void Interpreter::visit(expr::Assign& assign) {
  evaluate(*assign.value_);
  environment_->assign(assign.name_, value_);
}

void Interpreter::visit(expr::Binary& binary) {
  evaluate(*binary.left_);
  auto left = std::move(value_);

  evaluate(*binary.right_);
  auto right = std::move(value_);

  switch (binary.op_.get_type()) {
    case TokenType::BANG_EQUAL:
      value_ = Object{left != right};
      break;
    case TokenType::EQUAL_EQUAL:
      value_ = Object{left == right};
      break;
    case TokenType::GREATER:
      check_number_operands(binary.op_, left, right);
      value_ = Object{left.get<Object::Number>() > right.get<Object::Number>()};
      break;
    case TokenType::GREATER_EQUAL:
      check_number_operands(binary.op_, left, right);
      value_ =
          Object{left.get<Object::Number>() >= right.get<Object::Number>()};
      break;
    case TokenType::LESS:
      check_number_operands(binary.op_, left, right);
      value_ = Object{left.get<Object::Number>() < right.get<Object::Number>()};
      break;
    case TokenType::LESS_EQUAL:
      check_number_operands(binary.op_, left, right);
      value_ =
          Object{left.get<Object::Number>() <= right.get<Object::Number>()};
      break;
    case TokenType::MINUS:
      check_number_operands(binary.op_, left, right);
      value_ = Object{left.get<Object::Number>() - right.get<Object::Number>()};
      break;
    case TokenType::PLUS:
      if (left.is<Object::Number>() && right.is<Object::Number>()) {
        value_ =
            Object{left.get<Object::Number>() + right.get<Object::Number>()};
        break;
      }

      if (left.is<Object::String>() && right.is<Object::String>()) {
        value_ =
            Object{left.get<Object::String>() + right.get<Object::String>()};
        break;
      }

      throw RuntimeError(binary.op_,
                         "Operands must be two numbers or two strings.");
      break;
    case TokenType::SLASH:
      check_number_operands(binary.op_, left, right);
      value_ = Object{left.get<Object::Number>() / right.get<Object::Number>()};
      break;
    case TokenType::STAR:
      check_number_operands(binary.op_, left, right);
      value_ = Object{left.get<Object::Number>() * right.get<Object::Number>()};
      break;
    default:
      break;
  }
}

void Interpreter::visit(expr::Grouping& grouping) { evaluate(*grouping.expr_); }

void Interpreter::visit(expr::Literal& literal) { value_ = literal.value_; }

void Interpreter::visit(expr::Logical& logical) {
  evaluate(*logical.left_);

  if (logical.op_.get_type() == TokenType::OR) {
    if (is_truthy(value_)) {
      return;
    }
  } else {
    if (!is_truthy(value_)) {
      return;
    }
  }

  evaluate(*logical.right_);
}

void Interpreter::visit(expr::Unary& unary) {
  evaluate(*unary.right_);
  auto right = std::move(value_);

  switch (unary.op_.get_type()) {
    case TokenType::BANG:
      value_ = Object{!is_truthy(right)};
      break;
    case TokenType::MINUS:
      check_number_operand(unary.op_, right);
      value_ = Object{-right.get<Object::Number>()};
      break;
    default:
      break;
  }
}

void Interpreter::visit(expr::Variable& variable) {
  value_ = environment_->get(variable.name_);
}

void Interpreter::check_number_operand(const Token& op, const Object& operand) {
  if (operand.is<Object::Number>()) {
    return;
  }

  throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token& op, const Object& left,
                                        const Object& right) {
  if (left.is<Object::Number>() && right.is<Object::Number>()) {
    return;
  }

  throw RuntimeError(op, "Operands must be numbers.");
}

bool Interpreter::is_truthy(Object& value) {
  if (value.is<Object::Nil>()) {
    return false;
  }
  if (value.is<Object::Boolean>()) {
    return value.get<Object::Boolean>();
  }

  return true;
}
}  // namespace lox::treewalk
