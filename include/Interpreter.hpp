#pragma once

#include <vector>

#include "Environment.hpp"
#include "Expr.hpp"
#include "Stmt.hpp"

namespace lox::treewalk {
class Interpreter : public expr::Visitor, public stmt::Visitor {
 public:
  Interpreter();
  void interpret(const std::vector<stmt::Stmt::Ptr>& statements);
  void execute_block(const std::vector<stmt::Stmt::Ptr>& statements,
                     std::unique_ptr<Environment> environment);

 public:
  Object value_{};
  bool is_returning_ = false;

 private:
  void execute(const stmt::Stmt::Ptr& stmt);
  void visit(stmt::Block& block) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Function& function) override;
  void visit(stmt::If& if_) override;
  void visit(stmt::Print& print) override;
  void visit(stmt::Return& return_) override;
  void visit(stmt::Var& var) override;
  void visit(stmt::While& while_) override;

  void evaluate(const expr::Expr::Ptr& expr);
  void visit(expr::Assign& assign) override;
  void visit(expr::Binary& binary) override;
  void visit(expr::Call& call) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Logical& logical) override;
  void visit(expr::Unary& unary) override;
  void visit(expr::Variable& variable) override;

  static void check_number_operand(const Token& op, const Object& operand);
  static void check_number_operands(const Token& op, const Object& left,
                                    const Object& right);
  static bool is_truthy(Object& value);

 private:
  Environment globals_;
  std::unique_ptr<Environment> environment_;
};
}  // namespace lox::treewalk
