#include "resolver.hpp"

#include "treewalk.hpp"

namespace lox::treewalk {
void Resolver::resolve(const std::vector<Scope<Stmt>>& statements) {
  for (const auto& statement : statements) {
    resolve(statement);
  }
}

void Resolver::resolve(const Scope<Stmt>& stmt) { stmt->accept(*this); }

void Resolver::visit(stmt::Block& block) {
  begin_scope();
  resolve(block.statements_);
  end_scope();
}

void Resolver::visit(stmt::Class& class_) {
  const ClassType enclosing_class = current_class_;
  current_class_ = ClassType::CLASS;

  declare(class_.name_);
  define(class_.name_);

  if (class_.superclass_) {
    auto& superclass = class_.superclass_.value();
    if (class_.name_.get_lexeme() == superclass.name_.get_lexeme()) {
      error(superclass.name_, "A class can't inherit from itself.");
    }

    current_class_ = ClassType::SUBCLASS;

    visit(superclass);

    begin_scope();
    scopes_.back().insert_or_assign("super", true);
  }

  begin_scope();

  ScopeMap& scope = scopes_.back();
  scope.insert_or_assign("this", true);

  for (const auto& method : class_.methods_) {
    FunctionType declaration = FunctionType::METHOD;
    if (method.name_.get_lexeme() == "init") {
      declaration = FunctionType::INITIALIZER;
    }

    resolve_function(method, declaration);
  }

  end_scope();

  if (class_.superclass_) {
    end_scope();
  }

  current_class_ = enclosing_class;
}

void Resolver::visit(stmt::Expression& expression) {
  resolve(expression.expr_);
}

void Resolver::visit(stmt::Function& function) {
  declare(function.name_);
  define(function.name_);

  resolve_function(function, FunctionType::FUNCTION);
}

void Resolver::visit(stmt::If& if_) {
  resolve(if_.condition_);
  resolve(if_.then_branch_);

  if (if_.else_branch_) {
    resolve(if_.else_branch_);
  }
}

void Resolver::visit(stmt::Print& print) { resolve(print.expr_); }

void Resolver::visit(stmt::Return& return_) {
  if (current_function_ == FunctionType::NONE) {
    error(return_.keyword_, "Can't return from top-level code.");
  }

  if (return_.value_) {
    if (current_function_ == FunctionType::INITIALIZER) {
      error(return_.keyword_, "Can't return a value from an initializer.");
    }

    resolve(return_.value_);
  }
}

void Resolver::visit(stmt::Var& var) {
  declare(var.name_);
  if (var.initializer_) {
    resolve(var.initializer_);
  }
  define(var.name_);
}

void Resolver::visit(stmt::While& while_) {
  resolve(while_.condition_);
  resolve(while_.body_);
}

void Resolver::resolve(const Scope<Expr>& expr) { expr->accept(*this); }

void Resolver::visit(expr::Assign& assign) {
  resolve(assign.value_);
  resolve_local(assign, assign.name_);
}

void Resolver::visit(expr::Binary& binary) {
  resolve(binary.left_);
  resolve(binary.right_);
}

void Resolver::visit(expr::Call& call) {
  resolve(call.callee_);

  for (const auto& argument : call.arguments_) {
    resolve(argument);
  }
}

void Resolver::visit(expr::Get& get) { resolve(get.object_); }

void Resolver::visit(expr::Grouping& grouping) { resolve(grouping.expr_); }

void Resolver::visit(expr::Literal& literal) {}

void Resolver::visit(expr::Logical& logical) {
  resolve(logical.left_);
  resolve(logical.right_);
}

void Resolver::visit(expr::Set& set) {
  resolve(set.value_);
  resolve(set.object_);
}

void Resolver::visit(expr::This& this_) {
  if (current_class_ == ClassType::NONE) {
    error(this_.keyword_, "Can't use 'this' outside of a class.");
    return;
  }

  resolve_local(this_, this_.keyword_);
}

void Resolver::visit(expr::Super& super) {
  if (current_class_ == ClassType::NONE) {
    error(super.keyword_, "Can't use 'super' outside of a class.");
  } else if (current_class_ != ClassType::SUBCLASS) {
    error(super.keyword_, "Can't use 'super' in a class with no superclass.");
  }

  resolve_local(super, super.keyword_);
}

void Resolver::visit(expr::Unary& unary) { resolve(unary.right_); }

void Resolver::visit(expr::Variable& variable) {
  if (scopes_.empty()) {
    return;
  }

  ScopeMap& scope = scopes_.back();
  const std::string& lexeme = variable.name_.get_lexeme();
  if (auto it = scope.find(lexeme); it != scope.end() && !it->second) {
    error(variable.name_, "Can't read local variable in its own initializer.");
  }

  resolve_local(variable, variable.name_);
}

void Resolver::begin_scope() { scopes_.emplace_back(); }

void Resolver::end_scope() { scopes_.pop_back(); }

void Resolver::declare(const Token& name) {
  if (scopes_.empty()) {
    return;
  }

  ScopeMap& scope = scopes_.back();
  if (scope.find(name.get_lexeme()) != scope.end()) {
    error(name, "Already a variable with this name in this scope.");
  }

  scope.insert_or_assign(name.get_lexeme(), false);
}

void Resolver::define(const Token& name) {
  if (scopes_.empty()) {
    return;
  }

  ScopeMap& scope = scopes_.back();
  scope.insert_or_assign(name.get_lexeme(), true);
}

void Resolver::resolve_local(expr::Expr& expr, const Token& name) {
  const int size = static_cast<int>(scopes_.size());
  for (int i = size - 1; i >= 0; i--) {
    const ScopeMap& scope = scopes_[i];

    if (scope.find(name.get_lexeme()) != scope.end()) {
      expr.depth_ = size - 1 - i;
      return;
    }
  }
}

void Resolver::resolve_function(const stmt::Function& function,
                                FunctionType type) {
  const FunctionType enclosing_function = current_function_;
  current_function_ = type;
  begin_scope();

  for (const auto& param : function.params_) {
    declare(param);
    define(param);
  }
  resolve(function.body_);

  end_scope();
  current_function_ = enclosing_function;
}
}  // namespace lox::treewalk
