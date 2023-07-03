#pragma once

#include <optional>
#include <utility>

#include "expr.hpp"

namespace lox::treewalk::stmt {
struct Block;
struct Class;
struct Expression;
struct Function;
struct If;
struct Print;
struct Return;
struct Var;
struct While;

struct Visitor {
  virtual ~Visitor() = default;

  Visitor() = default;
  Visitor(const Visitor&) = delete;
  Visitor& operator=(const Visitor&) = delete;
  Visitor(Visitor&&) = delete;
  Visitor& operator=(Visitor&&) = delete;

  virtual void visit(Block& block) = 0;
  virtual void visit(Class& class_) = 0;
  virtual void visit(Expression& expression) = 0;
  virtual void visit(Function& function) = 0;
  virtual void visit(If& if_) = 0;
  virtual void visit(Print& print) = 0;
  virtual void visit(Return& return_) = 0;
  virtual void visit(Var& var) = 0;
  virtual void visit(While& while_) = 0;
};

struct Stmt {
  virtual ~Stmt() = default;

  Stmt() = default;
  Stmt(const Stmt&) = delete;
  Stmt& operator=(const Stmt&) = delete;
  Stmt(Stmt&&) = default;
  Stmt& operator=(Stmt&&) = default;

  virtual void accept(Visitor& visitor) = 0;
};

struct Block : Stmt {
  explicit Block(std::vector<std::unique_ptr<Stmt>> statements)
      : statements{std::move(statements)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::vector<std::unique_ptr<Stmt>> statements;
};

struct Function : Stmt {
  Function(lox::Token name, std::vector<lox::Token> params,
           std::vector<std::unique_ptr<Stmt>> body)
      : name{std::move(name)},
        params{std::move(params)},
        body{std::move(body)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token name;
  std::vector<lox::Token> params;
  std::vector<std::unique_ptr<Stmt>> body;
};

struct Class : Stmt {
  Class(lox::Token name, std::optional<expr::Variable> superclass,
        std::vector<Function> methods)
      : name{std::move(name)},
        superclass{std::move(superclass)},
        methods{std::move(methods)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token name;
  std::optional<expr::Variable> superclass;
  std::vector<Function> methods;
};

struct Expression : Stmt {
  explicit Expression(std::unique_ptr<Expr> expr) : expr{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> expr;
};

struct If : Stmt {
  If(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch,
     std::unique_ptr<Stmt> else_branch)
      : condition{std::move(condition)},
        then_branch{std::move(then_branch)},
        else_branch{std::move(else_branch)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> then_branch;
  std::unique_ptr<Stmt> else_branch;
};

struct Print : Stmt {
  explicit Print(std::unique_ptr<Expr> expr) : expr{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> expr;
};

struct Return : Stmt {
  explicit Return(lox::Token keyword, std::unique_ptr<Expr> value)
      : keyword{std::move(keyword)}, value{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token keyword;
  std::unique_ptr<Expr> value;
};

struct Var : Stmt {
  Var(lox::Token name, std::unique_ptr<Expr> initializer)
      : name{std::move(name)}, initializer{std::move(initializer)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  lox::Token name;
  std::unique_ptr<Expr> initializer;
};

struct While : Stmt {
  While(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
      : condition{std::move(condition)}, body{std::move(body)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> body;
};
}  // namespace lox::treewalk::stmt

namespace lox::treewalk {
using Stmt = stmt::Stmt;
}
