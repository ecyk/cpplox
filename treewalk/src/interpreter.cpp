#include "interpreter.hpp"

#include <chrono>
#include <iostream>

#include "treewalk.hpp"

namespace lox::treewalk {
namespace {
Value clock_native(int /*arg_count*/, Value* /*args*/) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

  constexpr double ms_to_seconds = 1.0 / 1000;
  return static_cast<double>(ms) * ms_to_seconds;
}
}  // namespace

Interpreter::Interpreter()
    : environment_{make_environment()}, globals_{environment_} {
  environment_->define("clock", std::make_shared<Native>(clock_native, 0));
}

void Interpreter::interpret(std::vector<std::unique_ptr<Stmt>>& statements) {
  try {
    for (auto& statement : statements) {
      execute(statement);
    }
  } catch (const RuntimeError& e) {
    runtime_error(e);
  }

  statements_.insert(statements_.end(),
                     std::make_move_iterator(statements.begin()),
                     std::make_move_iterator(statements.end()));
}

void Interpreter::execute_block(
    const std::vector<std::unique_ptr<Stmt>>& statements,
    Environment* environment) {
  if (statements.empty()) {
    return_value_ = {};
    return;
  }

  Environment* previous = environment_;
  call_stack_.push_back(previous);

  // try {
  environment_ = environment;

  for (const auto& statement : statements) {
    execute(statement);
  }
  // } catch (...) {
  //   environment_ = previous;
  //   throw;
  // }

  environment_ = call_stack_.back();
  call_stack_.pop_back();
}

void Interpreter::free_environments() {
  for (auto it = environments_.begin(); it != environments_.end();) {
    Environment* environment = *it;
    environments_.erase(it++);
    delete environment;
  }
}

void Interpreter::execute(const std::unique_ptr<Stmt>& stmt) {
  if (is_returning_) {
    return;
  }

  stmt->accept(*this);
}

void Interpreter::visit(stmt::Block& block) {
  execute_block(block.statements, make_environment(environment_));
}

void Interpreter::visit(stmt::Class& class_) {
  Value superclass;
  if (class_.superclass) {
    superclass = look_up_variable(class_.superclass->name, *class_.superclass);
    if (!IS_CLASS(superclass)) {
      throw RuntimeError{class_.superclass->name,
                         "Superclass must be a class."};
    }
  }

  environment_->define(std::string{class_.name.lexeme}, {});

  if (!IS_NIL(superclass)) {
    environment_ = make_environment(environment_);
    environment_->define("super", superclass);
  }

  Methods methods;
  for (auto& method : class_.methods) {
    methods.insert_or_assign(
        std::string{method.name.lexeme},
        Function{environment_, &method, static_cast<int>(method.params.size()),
                 method.name.lexeme == "init"});
  }

  if (!IS_NIL(superclass)) {
    environment_ = environment_->get_enclosing();
  }

  auto class_object = std::make_shared<Class>(
      std::move(methods), &class_,
      IS_CLASS(superclass) ? AS_CLASS(superclass).get() : nullptr, 0);

  if (Function* initializer = find_method(*class_object, "init")) {
    class_object->arity = initializer->arity;
  }

  environment_->assign(class_.name, class_object);
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(expression.expr);
}

void Interpreter::visit(stmt::Function& function) {
  environment_->define(std::string{function.name.lexeme},
                       {std::make_shared<Function>(
                           environment_, &function,
                           static_cast<int>(function.params.size()), false)});
}

void Interpreter::visit(stmt::If& if_) {
  if (!is_falsey(evaluate(if_.condition))) {
    execute(if_.then_branch);
  } else if (if_.else_branch) {
    execute(if_.else_branch);
  }
}

void Interpreter::visit(stmt::Print& print) {
  print_value(evaluate(print.expr));
  std::cout << '\n';
}

void Interpreter::visit(stmt::Return& return_) {
  if (return_.value) {
    evaluate(return_.value);
  } else {
    return_value_ = {};
  }

  is_returning_ = true;
  // throw Return(value);
}

void Interpreter::visit(stmt::Var& var) {
  if (var.initializer) {
    evaluate(var.initializer);
  } else {
    return_value_ = {};
  }

  environment_->define(std::string{var.name.lexeme}, return_value_);
}

void Interpreter::visit(stmt::While& while_) {
  while (!is_returning_) {
    if (is_falsey(evaluate(while_.condition))) {
      return;
    }
    execute(while_.body);
  }
}

Value& Interpreter::evaluate(const std::unique_ptr<Expr>& expr) {
  expr->accept(*this);
  return return_value_;
}

void Interpreter::visit(expr::Assign& assign) {
  const Value& value = evaluate(assign.value);
  const int distance = assign.depth;
  if (distance >= 0) {
    environment_->assign_at(distance, assign.name, value);
  } else {
    globals_->assign(assign.name, value);
  }
}

void Interpreter::visit(expr::Binary& binary) {
  Value left = evaluate(binary.left);
  const Value& right = evaluate(binary.right);

  switch (binary.op.type) {
    case TOKEN_BANG_EQUAL:
      return_value_ = {!(left == right)};
      break;
    case TOKEN_EQUAL_EQUAL:
      return_value_ = {left == right};
      break;
    case TOKEN_GREATER:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) > AS_NUMBER(right)};
      break;
    case TOKEN_GREATER_EQUAL:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) >= AS_NUMBER(right)};
      break;
    case TOKEN_LESS:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) < AS_NUMBER(right)};
      break;
    case TOKEN_LESS_EQUAL:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) <= AS_NUMBER(right)};
      break;
    case TOKEN_MINUS:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) - AS_NUMBER(right)};
      break;
    case TOKEN_PLUS:
      if (IS_NUMBER(left) && IS_NUMBER(right)) {
        return_value_ = {AS_NUMBER(left) + AS_NUMBER(right)};
        break;
      }
      if (IS_STRING(left) && IS_STRING(right)) {
        return_value_ = {AS_STRING(left) + AS_STRING(right)};
        break;
      }

      throw RuntimeError{binary.op,
                         "Operands must be two numbers or two strings."};
    case TOKEN_SLASH:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) / AS_NUMBER(right)};
      break;
    case TOKEN_STAR:
      check_number_operands(binary.op, left, right);
      return_value_ = {AS_NUMBER(left) * AS_NUMBER(right)};
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Call& call) {
  const Value callee = evaluate(call.callee);

  std::vector<Value> arguments;
  arguments.reserve(call.arguments.size());
  for (const auto& argument : call.arguments) {
    arguments.push_back(evaluate(argument));
  }

  return_value_ = call_value(callee, arguments, call.paren);
  is_returning_ = false;
}

void Interpreter::visit(expr::Get& get) {
  const Value& value = evaluate(get.object);
  if (IS_INSTANCE(value)) {
    const auto& instance = AS_INSTANCE(value);

    if (Value* field = find_field(*instance, get.name.lexeme)) {
      return_value_ = *field;
      return;
    }

    if (Function* method = find_method(*instance->class_, get.name.lexeme)) {
      return_value_ = {bind_function(method, instance)};
      return;
    }

    throw RuntimeError{
        get.name, "Undefined property '" + std::string{get.name.lexeme} + "'."};
  }

  throw RuntimeError{get.name, "Only instances have properties."};
}

void Interpreter::visit(expr::Grouping& grouping) { evaluate(grouping.expr); }

void Interpreter::visit(expr::Literal& literal) {
  return_value_ = literal.value;
}

void Interpreter::visit(expr::Logical& logical) {
  const Value& left = evaluate(logical.left);

  if (logical.op.type == TOKEN_OR) {
    if (!is_falsey(left)) {
      return;
    }
  } else {
    if (is_falsey(left)) {
      return;
    }
  }

  evaluate(logical.right);
}

void Interpreter::visit(expr::Set& set) {
  Value& object = evaluate(set.object);

  if (!IS_INSTANCE(object)) {
    throw RuntimeError{set.name, "Only instances have fields."};
  }

  AS_INSTANCE(object)->fields.insert_or_assign(std::string{set.name.lexeme},
                                               evaluate(set.value));
}

void Interpreter::visit(expr::This& this_) {
  return_value_ = look_up_variable(this_.keyword, this_);
}

void Interpreter::visit(expr::Super& super) {
  Function* method = find_method(
      *AS_CLASS(environment_->get_at(super.depth, {TOKEN_SUPER, "super", 0})),
      super.method.lexeme);

  if (method == nullptr) {
    throw RuntimeError{
        super.method,
        "Undefined property '" + std::string{super.method.lexeme} + "'."};
  }

  const Value& this_ =
      environment_->get_at(super.depth - 1, {TOKEN_THIS, "this", 0});

  return_value_ = {bind_function(method, AS_INSTANCE(this_))};
}

void Interpreter::visit(expr::Unary& unary) {
  const Value& right = evaluate(unary.right);

  switch (unary.op.type) {
    case TOKEN_BANG:
      return_value_ = {is_falsey(right)};
      break;
    case TOKEN_MINUS:
      check_number_operand(unary.op, right);
      return_value_ = {-AS_NUMBER(right)};
      break;
    default:
      return_value_ = {};
      break;
  }
}

void Interpreter::visit(expr::Variable& variable) {
  return_value_ = look_up_variable(variable.name, variable);
}

const Value& Interpreter::look_up_variable(const lox::Token& name,
                                           const expr::Expr& expr) {
  const int distance = expr.depth;
  if (distance >= 0) {
    return environment_->get_at(distance, name);
  }

  return globals_->get(name);
}

void Interpreter::check_number_operand(const lox::Token& op,
                                       const Value& operand) {
  if (IS_NUMBER(operand)) {
    return;
  }

  throw RuntimeError{op, "Operand must be a number."};
}

void Interpreter::check_number_operands(const lox::Token& op, const Value& left,
                                        const Value& right) {
  if (IS_NUMBER(left) && IS_NUMBER(right)) {
    return;
  }

  throw RuntimeError{op, "Operands must be numbers."};
}

Value* Interpreter::find_field(Instance& instance, std::string_view name) {
  if (auto it = instance.fields.find(name); it != instance.fields.end()) {
    return &it->second;
  }

  return nullptr;
}

Function* Interpreter::find_method(Class& class_, std::string_view name) {
  if (auto it = class_.methods.find(name); it != class_.methods.end()) {
    return &it->second;
  }

  if (class_.superclass != nullptr) {
    return find_method(*class_.superclass, name);
  }

  return nullptr;
}

std::shared_ptr<Function> Interpreter::bind_function(
    Function* function, const std::shared_ptr<Instance>& instance) {
  auto* closure = make_environment(function->closure);
  closure->define("this", {instance});
  return std::make_shared<Function>(closure, function->declaration,
                                    function->arity, function->is_initializer);
}

Value Interpreter::call_class(const std::shared_ptr<Class>& class_,
                              std::vector<Value> arguments) {
  auto instance = std::make_shared<Instance>(class_.get());

  if (Function* initializer = find_method(*class_, "init")) {
    call_function(bind_function(initializer, instance), std::move(arguments));
  }

  return instance;
}

Value Interpreter::call_function(const std::shared_ptr<Function>& function,
                                 std::vector<Value> arguments) {
  auto* environment = make_environment(function->closure);

  for (size_t i = 0; i < function->declaration->params.size(); i++) {
    environment->define(std::string{function->declaration->params[i].lexeme},
                        arguments[i]);
  }

  // try {
  execute_block(function->declaration->body, environment);
  // } catch (const Return& return_value) {
  //   return return_value.value_;
  // }

  if (function->is_initializer) {
    return function->closure->get_at(0, {TOKEN_THIS, "this", 0});
  }

  if (!is_returning_) {
    return {};
  }

  return return_value_;
}

Value Interpreter::call_native(const std::shared_ptr<Native>& native,
                               std::vector<Value> arguments) {
  return native->function(static_cast<int>(arguments.size()), arguments.data());
}

Value Interpreter::call_value(const Value& callee, std::vector<Value> arguments,
                              const lox::Token& token) {
  auto check_arity = [&](int arity) {
    if (arity != static_cast<int>(arguments.size())) {
      throw RuntimeError{token, "Expected " + std::to_string(arity) +
                                    " arguments but got " +
                                    std::to_string(arguments.size()) + "."};
    }
  };

  if (IS_CLASS(callee)) {
    check_arity(AS_CLASS(callee)->arity);
    return call_class(AS_CLASS(callee), arguments);
  }
  if (IS_FUNCTION(callee)) {
    check_arity(AS_FUNCTION(callee)->arity);
    return call_function(AS_FUNCTION(callee), arguments);
  }
  if (IS_NATIVE(callee)) {
    check_arity(AS_NATIVE(callee)->arity);
    return call_native(AS_NATIVE(callee), arguments);
  }

  throw RuntimeError{token, "Can only call functions and classes."};
}

void Interpreter::mark_function_closure(
    const std::shared_ptr<Function>& function) {
  mark_environment(function->closure);
}

void Interpreter::mark_class_closure(const std::shared_ptr<Class>& class_) {
  if (!class_->methods.empty()) {
    mark_environment(class_->methods.begin()->second.closure);
  }
}

void Interpreter::mark_environment(Environment* environment) {
  if (environment == nullptr) {
    return;
  }
  if (environment->is_marked_) {
    return;
  }

  environment->is_marked_ = true;

  for (const auto& [_, value] : environment->get_values()) {
    if (IS_FUNCTION(value)) {
      mark_function_closure(AS_FUNCTION(value));
    } else if (IS_CLASS(value)) {
      mark_class_closure(AS_CLASS(value));
    }
  }

  mark_environment(environment->get_enclosing());
}

Environment* Interpreter::make_environment(Environment* enclosing) {
  if (environments_created_++ > COLLECT_ENVIRONMENTS_THRESHOLD) {
    collect_environments(enclosing);
  }

  auto* environment = new Environment{enclosing};
  environments_.push_back(environment);
  return environment;
}

void Interpreter::collect_environments(Environment* enclosing) {
  mark_environment(globals_);
  mark_environment(environment_);
  mark_environment(enclosing);

  if (IS_FUNCTION(return_value_)) {
    mark_function_closure(AS_FUNCTION(return_value_));
  } else if (IS_CLASS(return_value_)) {
    mark_class_closure(AS_CLASS(return_value_));
  }

  for (Environment* environment : call_stack_) {
    mark_environment(environment);
  }

  for (auto it = environments_.begin(); it != environments_.end();) {
    if ((*it)->is_marked_) {
      (*it)->is_marked_ = false;
      it++;
    } else {
      Environment* unreached = *it;
      environments_.erase(it++);
      delete unreached;
    }
  }
}
}  // namespace lox::treewalk
