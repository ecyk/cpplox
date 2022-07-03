#pragma once

#include <vector>

#include "Expr.hpp"

namespace lox::treewalk::stmt {
class Block;
class Expression;
class Print;
class Var;

class Visitor {
 public:
  virtual ~Visitor() = default;

  virtual void visit(Block& block) = 0;
  virtual void visit(Expression& expression) = 0;
  virtual void visit(Print& print) = 0;
  virtual void visit(Var& var) = 0;
};

class Stmt {
 public:
  using Ptr = std::unique_ptr<Stmt>;

  virtual ~Stmt() = default;

  virtual void accept(Visitor& visitor) = 0;
};

class Block : public Stmt {
 public:
  explicit Block(std::vector<Ptr> statements)
      : statements_{std::move(statements)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::vector<Ptr> statements_;
};

class Expression : public Stmt {
 public:
  explicit Expression(expr::Expr::Ptr expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  expr::Expr::Ptr expr_;
};

class Print : public Stmt {
 public:
  explicit Print(expr::Expr::Ptr expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  expr::Expr::Ptr expr_;
};

class Var : public Stmt {
 public:
  explicit Var(Token name, expr::Expr::Ptr initializer)
      : name_{std::move(name)}, initializer_{std::move(initializer)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  expr::Expr::Ptr initializer_;
};
}  // namespace lox::treewalk::stmt
