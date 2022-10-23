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
      Callable{[&](const Arguments&) {
                 auto ms =
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

                 constexpr double ms_to_seconds = 1.0 / 1000;
                 return_value_ = static_cast<double>(ms) * ms_to_seconds;
               },
               0, Callable::Type::Native});
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
    if (is_returning_) {
      break;
    }
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
  environment_->define(class_.name_.get_lexeme(), {});
  environment_->assign(class_.name_, make_class(class_));
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(expression.expr_);
}

void Interpreter::visit(stmt::Function& function) {
  environment_->define(function.name_.get_lexeme(),
                       make_function(function, environment_));
}

void Interpreter::visit(stmt::If& if_) {
  if (is_truthy(evaluate(if_.condition_))) {
    execute(if_.then_branch_);
  } else if (if_.else_branch_) {
    execute(if_.else_branch_);
  }
}

void Interpreter::visit(stmt::Print& print) {
  const Object& value = evaluate(print.expr_);
  std::cout << stringify(value) << '\n';
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

Object& Interpreter::evaluate(const Scope<Expr>& expr) {
  expr->accept(*this);

  return return_value_;
}

void Interpreter::visit(expr::Assign& assign) {
  const Object& value = evaluate(assign.value_);

  int distance = assign.depth_;
  if (distance >= 0) {
    environment_->assign_at(distance, assign.name_, value);
  } else {
    globals_->assign(assign.name_, value);
  }
}

void Interpreter::visit(expr::Binary& binary) {
  Object left = evaluate(binary.left_);
  const Object& right = evaluate(binary.right_);

  switch (binary.op_.get_type()) {
    case TokenType::BANG_EQUAL:
      return_value_ = !is_equal(left, right);
      break;
    case TokenType::EQUAL_EQUAL:
      return_value_ = is_equal(left, right);
      break;
    case TokenType::GREATER:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) > as<Number>(right);
      break;
    case TokenType::GREATER_EQUAL:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) >= as<Number>(right);
      break;
    case TokenType::LESS:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) < as<Number>(right);
      break;
    case TokenType::LESS_EQUAL:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) <= as<Number>(right);
      break;
    case TokenType::MINUS:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) - as<Number>(right);
      break;
    case TokenType::PLUS:
      if (is<Number>(left) && is<Number>(right)) {
        return_value_ = as<Number>(left) + as<Number>(right);
        break;
      }
      if (is<String>(left) && is<String>(right)) {
        return_value_ = as<String>(left) + as<String>(right);
        break;
      }

      throw RuntimeError(binary.op_,
                         "Operands must be two numbers or two strings.");
      break;
    case TokenType::SLASH:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) / as<Number>(right);
      break;
    case TokenType::STAR:
      check_number_operands(binary.op_, left, right);
      return_value_ = as<Number>(left) * as<Number>(right);
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Call& call) {
  Object callee = evaluate(call.callee_);

  std::vector<Object> arguments;
  for (const auto& argument : call.arguments_) {
    arguments.push_back(evaluate(argument));
  }

  if (!is<Callable>(callee)) {
    throw RuntimeError(call.paren_, "Can only call functions and classes.");
  }

  const auto& callable = as<Callable>(callee);

  if (arguments.size() != callable.arity) {
    throw RuntimeError(call.paren_, "Expected " +
                                        std::to_string(callable.arity) +
                                        " arguments but got " +
                                        std::to_string(arguments.size()) + ".");
  }

  is_returning_ = false;
  callable.callback(arguments);
  is_returning_ = false;
}

void Interpreter::visit(expr::Get& get) {
  Object& object = evaluate(get.object_);
  if (is<Instance>(object)) {
    const auto& instance = as<Instance>(object);

    if (auto it = instance.fields->find(get.name_.get_lexeme());
        it != instance.fields->end()) {
      auto& field = it->second;
      return_value_ = field;
      return;
    }

    if (auto it = instance.methods->find(get.name_.get_lexeme());
        it != instance.methods->end()) {
      auto& method = it->second;

      auto closure = std::make_shared<Environment>(environment_);
      closure->define("this", object);
      return_value_ = make_function(
          *static_cast<stmt::Function*>(method.declaration), closure);
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
  const Object& left = evaluate(logical.left_);

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
  Object object = evaluate(set.object_);

  if (!is<Instance>(object)) {
    throw RuntimeError(set.name_, "Only instances have fields.");
  }

  const Object& value = evaluate(set.value_);
  const auto& instance = as<Instance>(object);
  instance.fields->insert_or_assign(set.name_.get_lexeme(), value);
}

void Interpreter::visit(expr::This& this_) {
  return_value_ = look_up_variable(this_.keyword_, this_);
}

void Interpreter::visit(expr::Unary& unary) {
  const Object& right = evaluate(unary.right_);

  switch (unary.op_.get_type()) {
    case TokenType::BANG:
      return_value_ = !is_truthy(right);
      break;
    case TokenType::MINUS:
      check_number_operand(unary.op_, right);
      return_value_ = -as<Number>(right);
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
  int distance = expr.depth_;
  if (distance >= 0) {
    return environment_->get_at(distance, name);
  }

  return globals_->get(name);
}

Callable Interpreter::make_function(stmt::Function& function,
                                    const Ref<Environment>& closure,
                                    bool is_initializer) {
  return Callable{
      [&, closure, is_initializer](const Arguments& arguments) {
        auto environment = std::make_shared<Environment>(closure);

        for (size_t i = 0; i < function.params_.size(); i++) {
          environment->define(function.params_[i].get_lexeme(), arguments[i]);
        }

        // try {
        execute_block(function.body_, environment);
        // } catch (const Return& return_value) {
        //   return return_value.value_;
        // }

        if (is_initializer) {
          return_value_ = closure->get_at(0, Token{TokenType::THIS, "this", 0});
        }
      },
      function.params_.size(), Callable::Type::FunctionOrMethod, &function,
      closure.get()};
}

Callable Interpreter::make_class(stmt::Class& class_) {
  Ref<Instance::Methods> methods = std::make_shared<Instance::Methods>();

  for (auto& method : class_.methods_) {
    const auto& name = method.name_.get_lexeme();
    methods->insert_or_assign(
        name, make_function(method, environment_, name == "init"));
  }

  size_t arity = 0;
  if (auto it = methods->find("init"); it != methods->end()) {
    arity = it->second.arity;
  }

  return Callable{
      [&, methods](const Arguments& arguments) {
        return_value_ = Instance{class_.name_.get_lexeme(), methods};

        if (auto it = methods->find("init"); it != methods->end()) {
          Callable& initializer = it->second;

          auto closure = std::make_shared<Environment>(environment_);
          closure->define("this", return_value_);
          make_function(*static_cast<stmt::Function*>(initializer.declaration),
                        closure, true)
              .callback(arguments);
        }
      },
      arity, Callable::Type::Class, &class_};
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

bool Interpreter::is_truthy(const Object& value) {
  if (is<Nil>(value)) {
    return false;
  }
  if (is<Boolean>(value)) {
    return as<Boolean>(value);
  }

  return true;
}

bool Interpreter::is_equal(const Object& left, const Object& right) {
  if (is<Number>(left) && is<Number>(right)) {
    return is_equal(as<Number>(left), as<Number>(right));
  }

  return left == right;
}

bool Interpreter::is_equal(Number a, Number b) {
  return std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) *
                                std::numeric_limits<Number>::epsilon();
}
}  // namespace lox::treewalk
