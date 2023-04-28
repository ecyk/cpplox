#include "resolver.hpp"

#include "treewalk.hpp"

namespace lox::treewalk {
void Resolver::resolve(const std::vector<std::unique_ptr<Stmt>>& statements) {
  for (const auto& statement : statements) {
    resolve(statement);
  }
}

void Resolver::resolve(const std::unique_ptr<Stmt>& stmt) {
  stmt->accept(*this);
}

void Resolver::visit(stmt::Block& block) {
  begin_scope();
  resolve(block.statements);
  end_scope();
}

void Resolver::visit(stmt::Class& class_) {
  const ClassType enclosing_class = current_class_;
  current_class_ = ClassType::CLASS;

  declare(class_.name);
  define(class_.name);

  if (class_.superclass) {
    auto& superclass = class_.superclass.value();
    if (class_.name.lexeme == superclass.name.lexeme) {
      error(superclass.name, "A class can't inherit from itself.");
    }

    current_class_ = ClassType::SUBCLASS;

    visit(superclass);

    begin_scope();
    scopes_.back().insert_or_assign("super", true);
  }

  begin_scope();

  ScopeMap& scope = scopes_.back();
  scope.insert_or_assign("this", true);

  for (const auto& method : class_.methods) {
    FunctionType declaration = FunctionType::METHOD;
    if (method.name.lexeme == "init") {
      declaration = FunctionType::INITIALIZER;
    }

    resolve_function(method, declaration);
  }

  end_scope();

  if (class_.superclass) {
    end_scope();
  }

  current_class_ = enclosing_class;
}

void Resolver::visit(stmt::Expression& expression) { resolve(expression.expr); }

void Resolver::visit(stmt::Function& function) {
  declare(function.name);
  define(function.name);

  resolve_function(function, FunctionType::FUNCTION);
}

void Resolver::visit(stmt::If& if_) {
  resolve(if_.condition);
  resolve(if_.then_branch);

  if (if_.else_branch) {
    resolve(if_.else_branch);
  }
}

void Resolver::visit(stmt::Print& print) { resolve(print.expr); }

void Resolver::visit(stmt::Return& return_) {
  if (current_function_ == FunctionType::NONE) {
    error(return_.keyword, "Can't return from top-level code.");
  }

  if (return_.value) {
    if (current_function_ == FunctionType::INITIALIZER) {
      error(return_.keyword, "Can't return a value from an initializer.");
    }

    resolve(return_.value);
  }
}

void Resolver::visit(stmt::Var& var) {
  declare(var.name);
  if (var.initializer) {
    resolve(var.initializer);
  }
  define(var.name);
}

void Resolver::visit(stmt::While& while_) {
  resolve(while_.condition);
  resolve(while_.body);
}

void Resolver::resolve(const std::unique_ptr<Expr>& expr) {
  expr->accept(*this);
}

void Resolver::visit(expr::Assign& assign) {
  resolve(assign.value);
  resolve_local(assign, assign.name);
}

void Resolver::visit(expr::Binary& binary) {
  resolve(binary.left);
  resolve(binary.right);
}

void Resolver::visit(expr::Call& call) {
  resolve(call.callee);

  for (const auto& argument : call.arguments) {
    resolve(argument);
  }
}

void Resolver::visit(expr::Get& get) { resolve(get.object); }

void Resolver::visit(expr::Grouping& grouping) { resolve(grouping.expr); }

void Resolver::visit(expr::Literal& literal) {}

void Resolver::visit(expr::Logical& logical) {
  resolve(logical.left);
  resolve(logical.right);
}

void Resolver::visit(expr::Set& set) {
  resolve(set.value);
  resolve(set.object);
}

void Resolver::visit(expr::This& this_) {
  if (current_class_ == ClassType::NONE) {
    error(this_.keyword, "Can't use 'this' outside of a class.");
    return;
  }

  resolve_local(this_, this_.keyword);
}

void Resolver::visit(expr::Super& super) {
  if (current_class_ == ClassType::NONE) {
    error(super.keyword, "Can't use 'super' outside of a class.");
  } else if (current_class_ != ClassType::SUBCLASS) {
    error(super.keyword, "Can't use 'super' in a class with no superclass.");
  }

  resolve_local(super, super.keyword);
}

void Resolver::visit(expr::Unary& unary) { resolve(unary.right); }

void Resolver::visit(expr::Variable& variable) {
  if (scopes_.empty()) {
    return;
  }

  ScopeMap& scope = scopes_.back();
  if (auto it = scope.find(variable.name.lexeme);
      it != scope.end() && !it->second) {
    error(variable.name, "Can't read local variable in its own initializer.");
  }

  resolve_local(variable, variable.name);
}

void Resolver::begin_scope() { scopes_.emplace_back(); }

void Resolver::end_scope() { scopes_.pop_back(); }

void Resolver::declare(const lox::Token& name) {
  if (scopes_.empty()) {
    return;
  }

  ScopeMap& scope = scopes_.back();
  if (scope.find(name.lexeme) != scope.end()) {
    error(name, "Already a variable with this name in this scope.");
  }

  scope.insert_or_assign(name.lexeme, false);
}

void Resolver::define(const lox::Token& name) {
  if (scopes_.empty()) {
    return;
  }

  ScopeMap& scope = scopes_.back();
  scope.insert_or_assign(name.lexeme, true);
}

void Resolver::resolve_local(expr::Expr& expr, const lox::Token& name) {
  const int size = static_cast<int>(scopes_.size());
  for (int i = size - 1; i >= 0; i--) {
    const ScopeMap& scope = scopes_[i];

    if (scope.find(name.lexeme) != scope.end()) {
      expr.depth = size - 1 - i;
      return;
    }
  }
}

void Resolver::resolve_function(const stmt::Function& function,
                                FunctionType type) {
  const FunctionType enclosing_function = current_function_;
  current_function_ = type;
  begin_scope();

  for (const auto& param : function.params) {
    declare(param);
    define(param);
  }
  resolve(function.body);

  end_scope();
  current_function_ = enclosing_function;
}
}  // namespace lox::treewalk
