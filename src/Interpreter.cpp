#include "Interpreter.hpp"

#include <chrono>
#include <iostream>

#include "Lox.hpp"
#include "LoxFunction.hpp"
#include "RuntimeError.hpp"

namespace lox::treewalk {
class ClockFn : public LoxCallable {
 public:
  size_t arity() override { return 0; }
  void call([[maybe_unused]] Interpreter& interpreter,
            [[maybe_unused]] const std::vector<Object>& arguments) override {
    interpreter.value_ = Object{static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count() /
        1000.0)};
  }

  std::string to_string() override { return "<native fn>"; }
};

Interpreter::Interpreter()
    : environment_{std::make_unique<Environment>(&globals_)} {
  globals_.define("clock", Object{std::make_shared<ClockFn>()});
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
  environment_->define(name, Object{std::make_shared<LoxFunction>(
                                 std::move(function), environment_.get())});
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
  std::cout << value_.stringify() << '\n';
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
  environment_->assign(assign.name_, value_);
}

void Interpreter::visit(expr::Binary& binary) {
  evaluate(binary.left_);
  auto left = std::move(value_);

  evaluate(binary.right_);
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

void Interpreter::visit(expr::Call& call) {
  evaluate(call.callee_);
  auto callee = std::move(value_);

  std::vector<Object> arguments;
  for (const auto& argument : call.arguments_) {
    evaluate(argument);
    arguments.push_back(std::move(value_));
  }

  if (!callee.is<Object::Callable>()) {
    throw RuntimeError(call.paren_, "Can only call functions and classes.");
  }

  auto& function = callee.get<Object::Callable>();
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
