#pragma once

#include "object.hpp"
#include "token.hpp"

namespace lox::treewalk::expr {
class Assign;
class Binary;
class Call;
class Get;
class Grouping;
class Literal;
class Logical;
class Set;
class This;
class Super;
class Unary;
class Variable;

class Visitor {
 public:
  virtual ~Visitor() = default;

  virtual void visit(Assign& assign) = 0;
  virtual void visit(Binary& binary) = 0;
  virtual void visit(Call& call) = 0;
  virtual void visit(Get& get) = 0;
  virtual void visit(Grouping& grouping) = 0;
  virtual void visit(Literal& literal) = 0;
  virtual void visit(Logical& logical) = 0;
  virtual void visit(Set& set) = 0;
  virtual void visit(This& this_) = 0;
  virtual void visit(Super& super) = 0;
  virtual void visit(Unary& unary) = 0;
  virtual void visit(Variable& variable) = 0;
};

class Expr {
 public:
  virtual ~Expr() = default;

  virtual void accept(Visitor& visitor) = 0;

  int depth_{-1};
};

class Assign : public Expr {
 public:
  Assign(Token name, Scope<Expr> value)
      : name_{std::move(name)}, value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  Scope<Expr> value_;
};

class Binary : public Expr {
 public:
  Binary(Scope<Expr> left, Token op, Scope<Expr> right)
      : left_{std::move(left)}, op_{std::move(op)}, right_{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> left_;
  Token op_;
  Scope<Expr> right_;
};

class Call : public Expr {
 public:
  Call(Scope<Expr> callee, Token paren, std::vector<Scope<Expr>> arguments)
      : callee_{std::move(callee)},
        paren_{std::move(paren)},
        arguments_{std::move(arguments)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> callee_;
  Token paren_;
  std::vector<Scope<Expr>> arguments_;
};

class Get : public Expr {
 public:
  Get(Scope<Expr> object, Token name)
      : object_{std::move(object)}, name_{std::move(name)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> object_;
  Token name_;
};

class Grouping : public Expr {
 public:
  explicit Grouping(Scope<Expr> expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> expr_;
};

class Literal : public Expr {
 public:
  explicit Literal(Object value) : value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Object value_;
};

class Logical : public Expr {
 public:
  Logical(Scope<Expr> left, Token op, Scope<Expr> right)
      : left_{std::move(left)}, op_{std::move(op)}, right_{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> left_;
  Token op_;
  Scope<Expr> right_;
};

class Set : public Expr {
 public:
  Set(Scope<Expr> object, Token name, Scope<Expr> value)
      : object_{std::move(object)},
        name_{std::move(name)},
        value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> object_;
  Token name_;
  Scope<Expr> value_;
};

class This : public Expr {
 public:
  This(Token keyword) : keyword_{std::move(keyword)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token keyword_;
};

class Super : public Expr {
 public:
  Super(Token keyword, Token method)
      : keyword_{std::move(keyword)}, method_{std::move(method)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token keyword_;
  Token method_;
};

class Unary : public Expr {
 public:
  Unary(Token op, Scope<Expr> right)
      : op_{std::move(op)}, right_{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token op_;
  Scope<Expr> right_;
};

class Variable : public Expr {
 public:
  Variable(Token name) : name_{std::move(name)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
};
}  // namespace lox::treewalk::expr

namespace lox::treewalk {
using Expr = expr::Expr;
}
