#include "AstPrinter.hpp"

namespace lox::treewalk {
std::string AstPrinter::print(const Scope<Expr>& expr) {
  expr->accept(*this);
  return str_;
}

void AstPrinter::visit(expr::Binary& binary) {
  str_ += "(" + binary.op_.get_lexeme();
  str_ += " ";
  binary.left_->accept(*this);
  str_ += " ";
  binary.right_->accept(*this);
  str_ += ")";
}

void AstPrinter::visit(expr::Grouping& grouping) {
  str_ += "(group";
  str_ += " ";
  grouping.expr_->accept(*this);
  str_ += ")";
}

void AstPrinter::visit(expr::Literal& literal) {
  str_ += stringify(literal.value_);
}

void AstPrinter::visit(expr::Unary& unary) {
  str_ += "(" + unary.op_.get_lexeme();
  str_ += " ";
  unary.right_->accept(*this);
  str_ += ")";
}
}  // namespace lox::treewalk
