#pragma once

#include "Environment.hpp"
#include "Stmt.hpp"

namespace lox::treewalk {
class Interpreter : public expr::Visitor, public stmt::Visitor {
 public:
  Interpreter();
  void interpret(const std::vector<Scope<Stmt>>& statements);
  void execute_block(const std::vector<Scope<Stmt>>& statements,
                     const Ref<Environment>& environment);

 private:
  void execute(const Scope<Stmt>& stmt);
  void visit(stmt::Block& block) override;
  void visit(stmt::Class& class_) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Function& function) override;
  void visit(stmt::If& if_) override;
  void visit(stmt::Print& print) override;
  void visit(stmt::Return& return_) override;
  void visit(stmt::Var& var) override;
  void visit(stmt::While& while_) override;

  LoxObject& evaluate(const Scope<Expr>& expr);
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

  LoxObject& look_up_variable(const Token& name, const expr::Expr& expr);

  static void check_number_operand(const Token& op, const LoxObject& operand);
  static void check_number_operands(const Token& op, const LoxObject& left,
                                    const LoxObject& right);
  static bool is_truthy(const LoxObject& value);
  static bool is_equal(const LoxObject& left, const LoxObject& right);
  static bool is_equal(LoxNumber a, LoxNumber b);

 private:
  LoxObject return_value_{};

  bool is_returning_{false};

  Ref<Environment> environment_;
  Environment* globals_;
};
}  // namespace lox::treewalk
