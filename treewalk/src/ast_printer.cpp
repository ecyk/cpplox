#include "ast_printer.hpp"

namespace lox::treewalk {
std::string AstPrinter::print(const std::unique_ptr<Expr>& expr) {
  expr->accept(*this);
  return str_;
}

void AstPrinter::visit(expr::Binary& binary) {
  str_ += "(" + std::string{binary.op.lexeme};
  str_ += " ";
  binary.left->accept(*this);
  str_ += " ";
  binary.right->accept(*this);
  str_ += ")";
}

void AstPrinter::visit(expr::Grouping& grouping) {
  str_ += "(group";
  str_ += " ";
  grouping.expr->accept(*this);
  str_ += ")";
}

void AstPrinter::visit(expr::Literal& literal) {
  // str_ += literal.value_.stringify();
}

void AstPrinter::visit(expr::Unary& unary) {
  str_ += "(" + std::string{unary.op.lexeme};
  str_ += " ";
  unary.right->accept(*this);
  str_ += ")";
}
}  // namespace lox::treewalk
