#pragma once

#include "Expr.hpp"

namespace lox::treewalk::stmt {
class Block;
class Class;
class Expression;
class Function;
class If;
class Print;
class Return;
class Var;
class While;

class Visitor {
 public:
  virtual ~Visitor() = default;

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

class Stmt {
 public:
  virtual ~Stmt() = default;

  virtual void accept(Visitor& visitor) = 0;
};

class Block : public Stmt {
 public:
  explicit Block(std::vector<Scope<Stmt>> statements)
      : statements_{std::move(statements)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  std::vector<Scope<Stmt>> statements_;
};

class Class : public Stmt {
 public:
  explicit Class(Token name, std::vector<Function> methods)
      : name_{std::move(name)}, methods_{std::move(methods)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  std::vector<Function> methods_;
};

class Expression : public Stmt {
 public:
  explicit Expression(Scope<Expr> expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> expr_;
};

class Function : public Stmt {
 public:
  Function(Token name, std::vector<Token> params, std::vector<Scope<Stmt>> body)
      : name_{std::move(name)},
        params_{std::move(params)},
        body_{std::move(body)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  std::vector<Token> params_;
  std::vector<Scope<Stmt>> body_;
};

class If : public Stmt {
 public:
  If(Scope<Expr> condition, Scope<Stmt> then_branch, Scope<Stmt> else_branch)
      : condition_{std::move(condition)},
        then_branch_{std::move(then_branch)},
        else_branch_{std::move(else_branch)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> condition_;
  Scope<Stmt> then_branch_;
  Scope<Stmt> else_branch_;
};

class Print : public Stmt {
 public:
  explicit Print(Scope<Expr> expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> expr_;
};

class Return : public Stmt {
 public:
  explicit Return(Token keyword, Scope<Expr> value)
      : keyword_{std::move(keyword)}, value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token keyword_;
  Scope<Expr> value_;
};

class Var : public Stmt {
 public:
  Var(Token name, Scope<Expr> initializer)
      : name_{std::move(name)}, initializer_{std::move(initializer)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  Scope<Expr> initializer_;
};

class While : public Stmt {
 public:
  While(Scope<Expr> condition, Scope<Stmt> body)
      : condition_{std::move(condition)}, body_{std::move(body)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Scope<Expr> condition_;
  Scope<Stmt> body_;
};
}  // namespace lox::treewalk::stmt

namespace lox::treewalk {
using Stmt = stmt::Stmt;
}
