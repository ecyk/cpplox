#include "interpreter.hpp"

#include <chrono>
#include <iostream>

#include "treewalk.hpp"

namespace lox::treewalk {
class Clock : public Function {
 public:
  Object call(const std::vector<Object>& /*arguments*/) override {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();

    constexpr double ms_to_seconds = 1.0 / 1000;
    return Object{static_cast<double>(ms) * ms_to_seconds};
  }
};

Interpreter::Interpreter()
    : environment_{std::make_shared<Environment>()},
      globals_{environment_.get()} {
  environment_->define("clock", Object{std::make_shared<Clock>()});
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
    execute(statement);
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
  Object* superclass = nullptr;
  if (class_.superclass_) {
    superclass =
        &look_up_variable(class_.superclass_->name_, *class_.superclass_);
    if (!superclass->is<Class>()) {
      throw RuntimeError(class_.superclass_->name_,
                         "Superclass must be a class.");
    }
  }

  environment_->define(class_.name_.get_lexeme(), {});

  if (superclass != nullptr) {
    environment_ = std::make_shared<Environment>(environment_);
    environment_->define("super", *superclass);
  }

  Class::Methods methods;
  for (auto& method : class_.methods_) {
    const auto& method_name = method.name_.get_lexeme();
    methods.insert_or_assign(
        method_name,
        Function{environment_, method_name == "init", &method, this});
  }

  if (superclass != nullptr) {
    environment_ = environment_->enclosing();
  }

  environment_->assign(
      class_.name_, Object{std::make_shared<Class>(std::move(methods),
                                                   superclass, &class_, this)});
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(expression.expr_);
}

void Interpreter::visit(stmt::Function& function) {
  environment_->define(
      function.name_.get_lexeme(),
      Object{std::make_shared<Function>(environment_, false, &function, this)});
}

void Interpreter::visit(stmt::If& if_) {
  if (evaluate(if_.condition_).is_truthy()) {
    execute(if_.then_branch_);
  } else if (if_.else_branch_) {
    execute(if_.else_branch_);
  }
}

void Interpreter::visit(stmt::Print& print) {
  std::cout << evaluate(print.expr_).stringify() << '\n';
}

void Interpreter::visit(stmt::Return& return_) {
  if (return_.value_) {
    evaluate(return_.value_);
  } else {
    return_value_ = {};
  }

  is_returning_ = true;
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
  while (!is_returning_) {
    if (!evaluate(while_.condition_).is_truthy()) {
      return;
    }
    execute(while_.body_);
  }
}

Object& Interpreter::evaluate(const Scope<Expr>& expr) {
  expr->accept(*this);

  return return_value_;
}

void Interpreter::visit(expr::Assign& assign) {
  const auto& value = evaluate(assign.value_);
  const int distance = assign.depth_;
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
      return_value_ = Object{!(left == right)};
      break;
    case TokenType::EQUAL_EQUAL:
      return_value_ = Object{left == right};
      break;
    case TokenType::GREATER:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() > right.as<Number>()};
      break;
    case TokenType::GREATER_EQUAL:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() >= right.as<Number>()};
      break;
    case TokenType::LESS:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() < right.as<Number>()};
      break;
    case TokenType::LESS_EQUAL:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() <= right.as<Number>()};
      break;
    case TokenType::MINUS:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() - right.as<Number>()};
      break;
    case TokenType::PLUS:
      if (left.is<Number>() && right.is<Number>()) {
        return_value_ = Object{left.as<Number>() + right.as<Number>()};
        break;
      }
      if (left.is<String>() && right.is<String>()) {
        return_value_ = Object{left.as<String>() + right.as<String>()};
        break;
      }

      throw RuntimeError(binary.op_,
                         "Operands must be two numbers or two strings.");
      break;
    case TokenType::SLASH:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() / right.as<Number>()};
      break;
    case TokenType::STAR:
      check_number_operands(binary.op_, left, right);
      return_value_ = Object{left.as<Number>() * right.as<Number>()};
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Call& call) {
  auto callee = evaluate(call.callee_);

  std::vector<Object> arguments;
  arguments.reserve(call.arguments_.size());
  for (const auto& argument : call.arguments_) {
    arguments.push_back(evaluate(argument));
  }

  if (!(callee.is<Function>() || callee.is<Class>())) {
    throw RuntimeError(call.paren_, "Can only call functions and classes.");
  }

  const int arity = callee.is<Function>() ? callee.as<Function>().arity()
                                          : callee.as<Class>().arity();

  if (arguments.size() != arity) {
    throw RuntimeError(call.paren_, "Expected " + std::to_string(arity) +
                                        " arguments but got " +
                                        std::to_string(arguments.size()) + ".");
  }

  auto value = callee.is<Function>() ? callee.as<Function>().call(arguments)
                                     : callee.as<Class>().call(arguments);
  if (!is_returning_ || !value.is<Nil>()) {
    return_value_ = std::move(value);
  }

  is_returning_ = false;
}

void Interpreter::visit(expr::Get& get) {
  const auto& object = evaluate(get.object_);
  if (object.is<Instance>()) {
    const auto& instance = object.as<Instance>();

    if (auto it = instance.fields_.find(get.name_.get_lexeme());
        it != instance.fields_.end()) {
      return_value_ = it->second;
      return;
    }

    if (const Function* method =
            instance.class_->find_method(get.name_.get_lexeme());
        method != nullptr) {
      return_value_ = Object{method->bind(object, this)};
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
    if (left.is_truthy()) {
      return;
    }
  } else {
    if (!left.is_truthy()) {
      return;
    }
  }

  evaluate(logical.right_);
}

void Interpreter::visit(expr::Set& set) {
  auto object = evaluate(set.object_);

  if (!object.is<Instance>()) {
    throw RuntimeError(set.name_, "Only instances have fields.");
  }

  object.as<Instance>().fields_.insert_or_assign(set.name_.get_lexeme(),
                                                 evaluate(set.value_));
}

void Interpreter::visit(expr::This& this_) {
  return_value_ = look_up_variable(this_.keyword_, this_);
}

void Interpreter::visit(expr::Super& super) {
  const Function* method =
      environment_->get_at(super.depth_, Token{TokenType::SUPER, "super", 0})
          .as<Class>()
          .find_method(super.method_.get_lexeme());

  if (method == nullptr) {
    throw RuntimeError(super.method_, "Undefined property '" +
                                          super.method_.get_lexeme() + "'.");
  }

  const auto& object =
      environment_->get_at(super.depth_ - 1, Token{TokenType::THIS, "this", 0});

  return_value_ = Object{method->bind(object, this)};
}

void Interpreter::visit(expr::Unary& unary) {
  const auto& right = evaluate(unary.right_);

  switch (unary.op_.get_type()) {
    case TokenType::BANG:
      return_value_ = Object{!right.is_truthy()};
      break;
    case TokenType::MINUS:
      check_number_operand(unary.op_, right);
      return_value_ = Object{-right.as<Number>()};
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Variable& variable) {
  return_value_ = look_up_variable(variable.name_, variable);
}

Object& Interpreter::look_up_variable(const Token& name,
                                      const expr::Expr& expr) {
  const int distance = expr.depth_;
  if (distance >= 0) {
    return environment_->get_at(distance, name);
  }

  return globals_->get(name);
}

void Interpreter::check_number_operand(const Token& op, const Object& operand) {
  if (operand.is<Number>()) {
    return;
  }

  throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token& op, const Object& left,
                                        const Object& right) {
  if (left.is<Number>() && right.is<Number>()) {
    return;
  }

  throw RuntimeError(op, "Operands must be numbers.");
}
}  // namespace lox::treewalk
