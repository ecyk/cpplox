#include "AstPrinter.hpp"

namespace lox::treewalk {
std::string AstPrinter::print(const expr::Expr::Ptr& expr) {
  expr->accept(*this);
  return str_;
}

void AstPrinter::visit(const expr::Binary& binary) {
  str_ += "(" + binary.op_.get_lexeme();
  str_ += " ";
  binary.left_->accept(*this);
  str_ += " ";
  binary.right_->accept(*this);
  str_ += ")";
}

void AstPrinter::visit(const expr::Grouping& grouping) {
  str_ += "(group";
  str_ += " ";
  grouping.expr_->accept(*this);
  str_ += ")";
}

void AstPrinter::visit(const expr::Literal& literal) {
  if (std::holds_alternative<expr::Literal::Nil>(literal.value_)) {
    str_ += "nil";
  } else if (std::holds_alternative<expr::Literal::String>(literal.value_)) {
    str_ += std::get<expr::Literal::String>(literal.value_);
  } else if (std::holds_alternative<expr::Literal::Number>(literal.value_)) {
    str_ += std::to_string(std::get<expr::Literal::Number>(literal.value_));
  } else if (std::holds_alternative<expr::Literal::Bool>(literal.value_)) {
    if (std::get<expr::Literal::Bool>(literal.value_)) {
      str_ += "true";
    } else {
      str_ += "false";
    }
  }
}

void AstPrinter::visit(const expr::Unary& unary) {
  str_ += "(" + unary.op_.get_lexeme();
  str_ += " ";
  unary.right_->accept(*this);
  str_ += ")";
}
}  // namespace lox::treewalk
