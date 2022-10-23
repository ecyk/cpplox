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

  Object& evaluate(const Scope<Expr>& expr);
  void visit(expr::Assign& assign) override;
  void visit(expr::Binary& binary) override;
  void visit(expr::Call& call) override;
  void visit(expr::Get& get) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Logical& logical) override;
  void visit(expr::Set& set) override;
  void visit(expr::This& this_) override;
  void visit(expr::Unary& unary) override;
  void visit(expr::Variable& variable) override;

  Object& look_up_variable(const Token& name, const expr::Expr& expr);

  Callable make_function(stmt::Function& function,
                         const Ref<Environment>& closure,
                         bool is_initializer = false);
  Callable make_class(stmt::Class& class_);

  static void check_number_operand(const Token& op, const Object& operand);
  static void check_number_operands(const Token& op, const Object& left,
                                    const Object& right);
  static bool is_truthy(const Object& value);
  static bool is_equal(const Object& left, const Object& right);
  static bool is_equal(Number a, Number b);

 private:
  Object return_value_{};

  bool is_returning_{false};

  Ref<Environment> environment_;
  Environment* globals_;
};
}  // namespace lox::treewalk
