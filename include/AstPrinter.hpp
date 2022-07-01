#pragma once

#include "Expr.hpp"

namespace lox::treewalk {
class AstPrinter : public expr::Visitor {
 public:
  std::string print(const expr::Expr::Ptr& expr);

 private:
  void visit(expr::Binary& binary) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Unary& unary) override;

 private:
  std::string str_;
};
}  // namespace lox::treewalk
