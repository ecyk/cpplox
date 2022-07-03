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

 private:
  void execute(stmt::Stmt& stmt);
  void execute_block(const std::vector<stmt::Stmt::Ptr>& statements,
                     std::shared_ptr<Environment> environment);
  void visit(stmt::Block& block);
  void visit(stmt::Print& print) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Var& var) override;

  void evaluate(expr::Expr& expr);
  void visit(expr::Assign& assign) override;
  void visit(expr::Binary& binary) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Unary& unary) override;
  void visit(expr::Variable& unary) override;

  static void check_number_operand(const Token& op, const Object& operand);
  static void check_number_operands(const Token& op, const Object& left,
                                    const Object& right);
  static bool is_truthy(Object& value);

 private:
  std::shared_ptr<Environment> environment_;

  Object value_;
};
}  // namespace lox::treewalk
