#pragma once

#include "Expr.hpp"

namespace lox::treewalk {
class Interpreter : public expr::Visitor {
 public:
  void interpret(expr::Expr::Ptr& expr);

  void visit(expr::Binary& binary) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Unary& unary) override;

 private:
  void evaluate(expr::Expr& expr);

  static void check_number_operand(const Token& op, const Object& operand);
  static void check_number_operands(const Token& op, const Object& left,
                                    const Object& right);
  static bool is_truthy(Object& value);

 private:
  Object value_;
};
}  // namespace lox::treewalk
