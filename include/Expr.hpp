#include <memory>
#include <variant>

#include "Token.hpp"

namespace lox::treewalk::expr {
class Binary;
class Grouping;
class Literal;
class Unary;

class Visitor {
 public:
  virtual ~Visitor() = default;

  virtual void visit(const Binary& binary) = 0;
  virtual void visit(const Grouping& grouping) = 0;
  virtual void visit(const Literal& literal) = 0;
  virtual void visit(const Unary& unary) = 0;
};

class Expr {
 public:
  using Ptr = std::unique_ptr<Expr>;

  virtual ~Expr() = default;

  virtual void accept(Visitor& visitor) = 0;
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
  using LiteralVariant = std::variant<std::string, double>;

  explicit Literal(std::string value) : value_{std::move(value)} {}
  explicit Literal(double value) : value_{value} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  LiteralVariant value_;
};

class Unary : public Expr {
 public:
  Unary(Token op, Ptr right) : op_{std::move(op)}, right_{std::move(right)} {}

  void accept(Visitor& visitor) override { visitor.visit(*this); }

  Token op_;
  Ptr right_;
};
}  // namespace lox::treewalk::expr
