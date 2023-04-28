#pragma once

#include "environment.hpp"
#include "stmt.hpp"

namespace lox::treewalk {
class Interpreter : public expr::Visitor, public stmt::Visitor {
 public:
  Interpreter();
  void interpret(std::vector<std::unique_ptr<Stmt>>& statements);
  void execute_block(const std::vector<std::unique_ptr<Stmt>>& statements,
                     std::shared_ptr<Environment> environment);

 private:
  void execute(const std::unique_ptr<Stmt>& stmt);
  void visit(stmt::Block& block) override;
  void visit(stmt::Class& class_) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Function& function) override;
  void visit(stmt::If& if_) override;
  void visit(stmt::Print& print) override;
  void visit(stmt::Return& return_) override;
  void visit(stmt::Var& var) override;
  void visit(stmt::While& while_) override;

  Value& evaluate(const std::unique_ptr<Expr>& expr);
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

  const Value& look_up_variable(const lox::Token& name, const expr::Expr& expr);

  static void check_number_operand(const lox::Token& op, const Value& operand);
  static void check_number_operands(const lox::Token& op, const Value& left,
                                    const Value& right);

  static Value* find_field(Instance& instance, std::string_view name);
  Function* find_method(Class& class_, std::string_view name);
  static std::shared_ptr<Function> bind_function(
      Function* function, const std::shared_ptr<Instance>& instance);
  Value call_class(const std::shared_ptr<Class>& class_,
                   std::vector<Value> arguments);
  Value call_function(const std::shared_ptr<Function>& function,
                      std::vector<Value> arguments);
  static Value call_native(const std::shared_ptr<Native>& native,
                           std::vector<Value> arguments);
  Value call_value(const Value& callee, std::vector<Value> arguments,
                   const lox::Token& token);

  Value return_value_{};
  bool is_returning_{};

  std::shared_ptr<Environment> environment_;
  Environment* globals_;

  std::vector<std::unique_ptr<Stmt>> statements_;
};
}  // namespace lox::treewalk
