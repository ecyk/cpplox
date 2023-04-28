#pragma once

#include <unordered_map>

#include "stmt.hpp"

namespace lox::treewalk {
class Resolver : public expr::Visitor, public stmt::Visitor {
  using ScopeMap = std::unordered_map<std::string_view, bool>;

 public:
  void resolve(const std::vector<std::unique_ptr<Stmt>>& statements);

 private:
  void resolve(const std::unique_ptr<Stmt>& stmt);
  void visit(stmt::Block& block) override;
  void visit(stmt::Class& class_) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Function& function) override;
  void visit(stmt::If& if_) override;
  void visit(stmt::Print& print) override;
  void visit(stmt::Return& return_) override;
  void visit(stmt::Var& var) override;
  void visit(stmt::While& while_) override;

  void resolve(const std::unique_ptr<Expr>& expr);
  void visit(expr::Assign& assign) override;
  void visit(expr::Binary& binary) override;
  void visit(expr::Call& call) override;
  void visit(expr::Get& get) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Logical& logical) override;
  void visit(expr::Set& set) override;
  void visit(expr::This& this_) override;
  void visit(expr::Super& super) override;
  void visit(expr::Unary& unary) override;
  void visit(expr::Variable& variable) override;

  void begin_scope();
  void end_scope();

  void declare(const lox::Token& name);
  void define(const lox::Token& name);

  void resolve_local(expr::Expr& expr, const lox::Token& name);

  enum class FunctionType { NONE, FUNCTION, INITIALIZER, METHOD };
  void resolve_function(const stmt::Function& function, FunctionType type);

  std::vector<ScopeMap> scopes_;
  FunctionType current_function_{FunctionType::NONE};

  enum class ClassType { NONE, CLASS, SUBCLASS };
  ClassType current_class_{ClassType::NONE};
};
}  // namespace lox::treewalk
