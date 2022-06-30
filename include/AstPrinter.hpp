#pragma once

#include "Expr.hpp"

namespace lox::treewalk {
class AstPrinter : public expr::Visitor {
 public:
  std::string print(const expr::Expr::Ptr& expr);

 private:
  void visit(const expr::Binary& binary) override;
  void visit(const expr::Grouping& grouping) override;
  void visit(const expr::Literal& literal) override;
  void visit(const expr::Unary& unary) override;

 private:
  std::string str_;
};
}  // namespace lox::treewalk
