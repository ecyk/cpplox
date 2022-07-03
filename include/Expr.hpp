#pragma once

#include <memory>

#include "Object.hpp"
#include "Token.hpp"

namespace lox::treewalk::expr {
class Assign;
class Binary;
class Grouping;
class Literal;
class Unary;
class Variable;

class Visitor {
 public:
  virtual ~Visitor() = default;

  virtual void visit(Assign& assign) = 0;
  virtual void visit(Binary& binary) = 0;
  virtual void visit(Grouping& grouping) = 0;
  virtual void visit(Literal& literal) = 0;
  virtual void visit(Unary& unary) = 0;
  virtual void visit(Variable& unary) = 0;
};

class Expr {
 public:
  using Ptr = std::unique_ptr<Expr>;

  virtual ~Expr() = default;

  virtual void accept(Visitor& visitor) = 0;
};

class Assign : public Expr {
 public:
  Assign(Token name, Ptr value)
      : name_{std::move(name)}, value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  Ptr value_;
};

class Binary : public Expr {
 public:
  Binary(Ptr left, Token op, Ptr right)
      : left_{std::move(left)}, op_{std::move(op)}, right_{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Ptr left_;
  Token op_;
  Ptr right_;
};

class Grouping : public Expr {
 public:
  explicit Grouping(Ptr expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Ptr expr_;
};

class Literal : public Expr {
 public:
  explicit Literal(Object value) : value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Object value_;
};

class Unary : public Expr {
 public:
  Unary(Token op, Ptr right) : op_{std::move(op)}, right_{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token op_;
  Ptr right_;
};

class Variable : public Expr {
 public:
  Variable(Token name) : name_{std::move(name)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
};
}  // namespace lox::treewalk::expr
