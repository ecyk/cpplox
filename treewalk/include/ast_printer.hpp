#pragma once

#include "expr.hpp"

namespace lox::treewalk {
class AstPrinter : public expr::Visitor {
 public:
  std::string print(const std::unique_ptr<Expr>& expr);

 private:
  void visit(expr::Binary& binary) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Unary& unary) override;

  std::string str_;
};
}  // namespace lox::treewalk
