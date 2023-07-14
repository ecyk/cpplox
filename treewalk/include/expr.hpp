#pragma once

#include <utility>

#include "scanner.hpp"
#include "value.hpp"

namespace lox::treewalk::expr {
struct Assign;
struct Binary;
struct Call;
struct Get;
struct Grouping;
struct Literal;
struct Logical;
struct Set;
struct This;
struct Super;
struct Unary;
struct Variable;

struct Visitor {
  virtual ~Visitor() = default;

  Visitor() = default;

  Visitor(const Visitor&) = delete;
  Visitor& operator=(const Visitor&) = delete;

  Visitor(Visitor&&) = delete;
  Visitor& operator=(Visitor&&) = delete;

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

struct Expr {
  virtual ~Expr() = default;

  Expr() = default;

  Expr(const Expr&) = delete;
  Expr& operator=(const Expr&) = delete;

  Expr(Expr&&) = default;
  Expr& operator=(Expr&&) = default;

  virtual void accept(Visitor& visitor) = 0;

  int depth{-1};
};

struct Assign : Expr {
  Assign(lox::Token name, std::unique_ptr<Expr> value)
      : name{std::move(name)}, value{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token name;
  std::unique_ptr<Expr> value;
};

struct Binary : Expr {
  Binary(std::unique_ptr<Expr> left, lox::Token op, std::unique_ptr<Expr> right)
      : left{std::move(left)}, op{std::move(op)}, right{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> left;
  lox::Token op;
  std::unique_ptr<Expr> right;
};

struct Call : Expr {
  Call(std::unique_ptr<Expr> callee, lox::Token paren,
       std::vector<std::unique_ptr<Expr>> arguments)
      : callee{std::move(callee)},
        paren{std::move(paren)},
        arguments{std::move(arguments)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> callee;
  lox::Token paren;
  std::vector<std::unique_ptr<Expr>> arguments;
};

struct Get : Expr {
  Get(std::unique_ptr<Expr> object, lox::Token name)
      : object{std::move(object)}, name{std::move(name)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> object;
  lox::Token name;
};

struct Grouping : Expr {
  explicit Grouping(std::unique_ptr<Expr> expr) : expr{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> expr;
};

struct Literal : Expr {
  explicit Literal(Value value) : value{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Value value;
};

struct Logical : Expr {
  Logical(std::unique_ptr<Expr> left, lox::Token op,
          std::unique_ptr<Expr> right)
      : left{std::move(left)}, op{std::move(op)}, right{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> left;
  lox::Token op;
  std::unique_ptr<Expr> right;
};

struct Set : Expr {
  Set(std::unique_ptr<Expr> object, lox::Token name,
      std::unique_ptr<Expr> value)
      : object{std::move(object)},
        name{std::move(name)},
        value{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> object;
  lox::Token name;
  std::unique_ptr<Expr> value;
};

struct This : Expr {
  explicit This(lox::Token keyword) : keyword{std::move(keyword)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token keyword;
};

struct Super : Expr {
  Super(lox::Token keyword, lox::Token method)
      : keyword{std::move(keyword)}, method{std::move(method)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token keyword;
  lox::Token method;
};

struct Unary : Expr {
  Unary(lox::Token op, std::unique_ptr<Expr> right)
      : op{std::move(op)}, right{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token op;
  std::unique_ptr<Expr> right;
};

struct Variable : Expr {
  explicit Variable(lox::Token name) : name{std::move(name)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token name;
};
}  // namespace lox::treewalk::expr

namespace lox::treewalk {
using Expr = expr::Expr;
}
