#include "Interpreter.hpp"

#include <iostream>

#include "Lox.hpp"
#include "RuntimeError.hpp"

namespace lox::treewalk {
void Interpreter::interpret(expr::Expr::Ptr& expr) {
  try {
    evaluate(*expr);
    std::cout << value_.stringify() << '\n';
  } catch (const RuntimeError& e) {
    runtime_error(e);
  }
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

void Interpreter::visit(expr::Literal& literal) {
  value_ = std::move(literal.value_);
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

void Interpreter::evaluate(expr::Expr& expr) { expr.accept(*this); }

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
