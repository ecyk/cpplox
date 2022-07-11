#pragma once

#include <vector>

#include "Expr.hpp"

namespace lox::treewalk::stmt {
class Block;
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

class Function : public Stmt {
 public:
  Function(Token name, std::vector<Token> params, std::vector<Ptr> body)
      : name_{std::move(name)},
        params_{std::move(params)},
        body_{std::move(body)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  std::vector<Token> params_;
  std::vector<Ptr> body_;
};

class If : public Stmt {
 public:
  If(expr::Expr::Ptr condition, Ptr then_branch, Ptr else_branch)
      : condition_{std::move(condition)},
        then_branch_{std::move(then_branch)},
        else_branch_{std::move(else_branch)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  expr::Expr::Ptr condition_;
  Ptr then_branch_;
  Ptr else_branch_;
};

class Print : public Stmt {
 public:
  explicit Print(expr::Expr::Ptr expr) : expr_{std::move(expr)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  expr::Expr::Ptr expr_;
};

class Return : public Stmt {
 public:
  explicit Return(Token keyword, expr::Expr::Ptr value)
      : keyword_{std::move(keyword)}, value_{std::move(value)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token keyword_;
  expr::Expr::Ptr value_;
};

class Var : public Stmt {
 public:
  Var(Token name, expr::Expr::Ptr initializer)
      : name_{std::move(name)}, initializer_{std::move(initializer)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token name_;
  expr::Expr::Ptr initializer_;
};

class While : public Stmt {
 public:
  While(expr::Expr::Ptr condition, Ptr body)
      : condition_{std::move(condition)}, body_{std::move(body)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  expr::Expr::Ptr condition_;
  Ptr body_;
};
}  // namespace lox::treewalk::stmt
