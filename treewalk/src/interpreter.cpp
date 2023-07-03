#include "interpreter.hpp"

#include <chrono>

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

Interpreter::Interpreter() {
  const StackObject environment{allocate_object<Environment>(), this};

  environment_ = static_cast<Environment*>(environment);
  globals_ = static_cast<Environment*>(environment);

  const StackObject clock{allocate_object<ObjNative>(clock_native, 0), this};
  environment_->define("clock", static_cast<ObjNative*>(clock));
}

void Interpreter::interpret(std::vector<std::unique_ptr<Stmt>> statements) {
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

void Interpreter::free_objects() const {
  Obj* object = objects_;
  while (object != nullptr) {
    Obj* next = object->next_object;
#ifdef DEBUG_LOG_GC
    std::cout << static_cast<void*>(object) << " free type "
              << static_cast<int>(object->type) << '\n';
#endif
    delete object;
    object = next;
  }
}

void Interpreter::execute_block(
    const std::vector<std::unique_ptr<Stmt>>& statements,
    Environment* environment) {
  if (statements.empty()) {
    return_value_ = {};
    return;
  }

  Environment* previous = environment_;

  // try {
  environment_ = environment;

  for (const auto& statement : statements) {
    execute(statement);
  }
  // } catch (...) {
  //   environment_ = previous;
  //   throw;
  // }

  environment_ = previous;
}

void Interpreter::execute(const std::unique_ptr<Stmt>& stmt) {
  if (is_returning_) {
    return;
  }

  stmt->accept(*this);
}

void Interpreter::visit(stmt::Block& block) {
  const StackObject environment{allocate_object<Environment>(environment_),
                                this};
  execute_block(block.statements, static_cast<Environment*>(environment));
}

void Interpreter::visit(stmt::Class& class_) {
  ObjClass* superclass{};
  if (class_.superclass) {
    const Value& variable =
        look_up_variable(class_.superclass->name, *class_.superclass);

    if (IS_CLASS(variable)) {
      superclass = AS_CLASS(variable);
    } else {
      throw RuntimeError{class_.superclass->name,
                         "Superclass must be a class."};
    }
  }

  environment_->define(class_.name.lexeme, {});

  if (superclass != nullptr) {
    environment_ = allocate_object<Environment>(environment_);
    environment_->define("super", superclass);
  }

  Methods methods;
  for (auto& method : class_.methods) {
    methods.insert_or_assign(method.name.lexeme,
                             ObjFunction{environment_, &method,
                                         static_cast<int>(method.params.size()),
                                         method.name.lexeme == "init"});
  }

  auto* object =
      allocate_object<ObjClass>(std::move(methods), &class_, superclass, 0);

  if (superclass != nullptr) {
    environment_ = environment_->get_enclosing();
  }
  
  if (ObjFunction* initializer = find_method(object, "init")) {
    object->arity = initializer->arity;
  }

  environment_->assign(class_.name, object);
}

void Interpreter::visit(stmt::Expression& expression) {
  evaluate(expression.expr);
}

void Interpreter::visit(stmt::Function& function) {
  environment_->define(function.name.lexeme,
                       allocate_object<ObjFunction>(
                           environment_, &function,
                           static_cast<int>(function.params.size()), false));
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

  environment_->define(var.name.lexeme, return_value_);
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

  if (IS_OBJ(left)) {
    stack_.push_back(AS_OBJ(left));
  }

  const Value& right = evaluate(binary.right);

  if (IS_OBJ(left)) {
    stack_.pop_back();
  }

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

    if (Value* field = find_field(instance, get.name.lexeme)) {
      return_value_ = *field;
      return;
    }

    if (ObjFunction* method = find_method(instance->class_, get.name.lexeme)) {
      return_value_ = bind_function(method, instance);
      return;
    }

    throw RuntimeError{get.name,
                       "Undefined property '" + get.name.lexeme + "'."};
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

  AS_INSTANCE(object)->fields.insert_or_assign(set.name.lexeme,
                                               evaluate(set.value));
}

void Interpreter::visit(expr::This& this_) {
  return_value_ = look_up_variable(this_.keyword, this_);
}

void Interpreter::visit(expr::Super& super) {
  ObjFunction* method = find_method(
      AS_CLASS(environment_->get_at(super.depth, {TOKEN_SUPER, "super", 0})),
      super.method.lexeme);

  if (method == nullptr) {
    throw RuntimeError{super.method,
                       "Undefined property '" + super.method.lexeme + "'."};
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
                                           const expr::Expr& expr) const {
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

Value* Interpreter::find_field(ObjInstance* instance, const std::string& name) {
  if (auto it = instance->fields.find(name); it != instance->fields.end()) {
    return &it->second;
  }

  return nullptr;
}

ObjFunction* Interpreter::find_method(ObjClass* class_,
                                      const std::string& name) {
  if (auto it = class_->methods.find(name); it != class_->methods.end()) {
    return &it->second;
  }

  if (class_->superclass != nullptr) {
    return find_method(class_->superclass, name);
  }

  return nullptr;
}

ObjFunction* Interpreter::bind_function(ObjFunction* function,
                                        ObjInstance* instance) {
  const StackObject environment{allocate_object<Environment>(function->closure),
                                this};
  auto* closure = static_cast<Environment*>(environment);
  closure->define("this", {instance});
  return allocate_object<ObjFunction>(closure, function->declaration,
                                      function->arity,
                                      function->is_initializer);
}

Value Interpreter::call_class(ObjClass* class_, std::vector<Value> arguments) {
  const StackObject instance{allocate_object<ObjInstance>(class_), this};

  if (ObjFunction* method = find_method(class_, "init")) {
    const StackObject initializer{
        bind_function(method, static_cast<ObjInstance*>(instance)), this};
    call_function(static_cast<ObjFunction*>(initializer), std::move(arguments));
  }

  return static_cast<ObjInstance*>(instance);
}

Value Interpreter::call_function(ObjFunction* function,
                                 std::vector<Value> arguments) {
  const StackObject environment{allocate_object<Environment>(function->closure),
                                this};

  for (size_t i = 0; i < function->declaration->params.size(); i++) {
    static_cast<Environment*>(environment)
        ->define(function->declaration->params[i].lexeme, arguments[i]);
  }

  // try {
  execute_block(function->declaration->body,
                static_cast<Environment*>(environment));
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

Value Interpreter::call_native(ObjNative* native,
                               std::vector<Value> arguments) {
  return native->function(static_cast<int>(arguments.size()), arguments.data());
}

Value Interpreter::call_value(const Value& callee, std::vector<Value> arguments,
                              const lox::Token& token) {
  if (!IS_OBJ(callee)) {
    throw RuntimeError{token, "Can only call functions and classes."};
  }

  auto check_arity = [&](int arity) {
    if (arity != static_cast<int>(arguments.size())) {
      throw RuntimeError{token, "Expected " + std::to_string(arity) +
                                    " arguments but got " +
                                    std::to_string(arguments.size()) + "."};
    }
  };

  const StackObject callable{AS_OBJ(callee), this};

  switch (AS_OBJ(callee)->type) {
    case Obj::Type::CLASS:
      check_arity(AS_CLASS(callee)->arity);
      return call_class(AS_CLASS(callee), arguments);
    case Obj::Type::FUNCTION:
      check_arity(AS_FUNCTION(callee)->arity);
      return call_function(AS_FUNCTION(callee), arguments);
    case Obj::Type::NATIVE:
      check_arity(AS_NATIVE(callee)->arity);
      return call_native(AS_NATIVE(callee), arguments);
    default:
      throw RuntimeError{token, "Can only call functions and classes."};
  }
}

void Interpreter::collect_garbage() {
#ifdef DEBUG_LOG_GC
  std::cout << "-- gc begin\n";
  const size_t before = bytes_allocated_;
#endif

  mark_roots();
  trace_references();
  sweep();

  next_gc_ = bytes_allocated_ * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  std::cout << "-- gc end\n";
  std::cout << "   collected " << before - bytes_allocated_ << " bytes (from "
            << before << " to " << bytes_allocated_ << ") next at " << next_gc_
            << '\n';
#endif
}

void Interpreter::mark_object(Obj* object) {
  if (object == nullptr) {
    return;
  }
  if (object->is_marked) {
    return;
  }

#ifdef DEBUG_LOG_GC
  std::cout << static_cast<void*>(object) << " mark ";
  print_value(Value{object});
  std::cout << '\n';
#endif

  object->is_marked = true;

  gray_stack_.push(object);
}

void Interpreter::mark_value(const Value& value) {
  if (IS_OBJ(value)) {
    mark_object(AS_OBJ(value));
  }
}

void Interpreter::mark_environment(Environment* environment) {
  if (environment == nullptr) {
    return;
  }
  if (environment->is_marked) {
    return;
  }

  environment->is_marked = true;

  for (const auto& [_, value] : environment->get_values()) {
    mark_value(value);
  }

  mark_environment(environment->get_enclosing());
}

void Interpreter::blacken_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  std::cout << static_cast<void*>(object) << " blacken ";
  print_value(Value{object});
  std::cout << '\n';
#endif

  switch (object->type) {
    case Obj::Type::CLASS: {
      auto* class_ = static_cast<ObjClass*>(object);
      if (!class_->methods.empty()) {
        auto& [_, method] = *class_->methods.begin();
        mark_environment(method.closure);
      }
      break;
    }
    case Obj::Type::FUNCTION: {
      auto* function = static_cast<ObjFunction*>(object);
      mark_environment(function->closure);
      break;
    }
    case Obj::Type::INSTANCE: {
      auto* instance = static_cast<ObjInstance*>(object);
      for (const auto& [_, field] : instance->fields) {
        mark_value(field);
      }
      break;
    }
    case Obj::Type::ENVIRONMENT: {
      auto* environment = static_cast<Environment*>(object);
      mark_environment(environment);
    } break;
    default:
      break;
  }
}

void Interpreter::mark_roots() {
  mark_environment(globals_);
  mark_environment(environment_);

  mark_value(return_value_);

  for (Obj* object : stack_) {
    mark_object(object);
  }
}

void Interpreter::trace_references() {
  while (!gray_stack_.empty()) {
    Obj* object = gray_stack_.top();
    gray_stack_.pop();
    blacken_object(object);
  }
}

void Interpreter::sweep() {
  Obj* previous = nullptr;
  Obj* object = objects_;
  while (object != nullptr) {
    if (object->is_marked) {
      object->is_marked = false;
      previous = object;
      object = object->next_object;
    } else {
      Obj* unreached = object;
      object = object->next_object;
      if (previous != nullptr) {
        previous->next_object = object;
      } else {
        objects_ = object;
      }

      switch (unreached->type) {
        case Obj::Type::CLASS:
          bytes_allocated_ -= sizeof(ObjClass);
          break;
        case Obj::Type::FUNCTION:
          bytes_allocated_ -= sizeof(ObjFunction);
          break;
        case Obj::Type::INSTANCE:
          bytes_allocated_ -= sizeof(ObjInstance);
          break;
        case Obj::Type::NATIVE:
          bytes_allocated_ -= sizeof(ObjNative);
          break;
        case Obj::Type::ENVIRONMENT:
          bytes_allocated_ -= sizeof(Environment);
      }

#ifdef DEBUG_LOG_GC
      std::cout << static_cast<void*>(unreached) << " free type "
                << static_cast<int>(unreached->type) << '\n';
#endif
      delete unreached;
    }
  }
}
}  // namespace lox::treewalk
