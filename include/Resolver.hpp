#pragma once

#include <unordered_map>

#include "Stmt.hpp"

namespace lox::treewalk {
class Resolver : public expr::Visitor, public stmt::Visitor {
  using Scope = std::unordered_map<std::string, bool>;

 public:
  void resolve(const std::vector<stmt::Stmt::Ptr>& statements);

 private:
  void resolve(const stmt::Stmt::Ptr& stmt);
  void visit(stmt::Block& block) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Function& function) override;
  void visit(stmt::If& if_) override;
  void visit(stmt::Print& print) override;
  void visit(stmt::Return& return_) override;
  void visit(stmt::Var& var) override;
  void visit(stmt::While& while_) override;

  void resolve(const expr::Expr::Ptr& expr);
  void visit(expr::Assign& assign) override;
  void visit(expr::Binary& binary) override;
  void visit(expr::Call& call) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Logical& logical) override;
  void visit(expr::Unary& unary) override;
  void visit(expr::Variable& variable) override;

  void begin_scope();
  void end_scope();

  void declare(const Token& name);
  void define(const Token& name);

  void resolve_local(expr::Expr& expr, const Token& name);

  enum class FunctionType { NONE, FUNCTION };
  void resolve_function(const stmt::Function& function, FunctionType type);

 private:
  std::vector<Scope> scopes_;
  FunctionType current_function_{FunctionType::NONE};
};
}  // namespace lox::treewalk
