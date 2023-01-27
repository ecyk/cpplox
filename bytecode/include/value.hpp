#pragma once

#include "common.hpp"

namespace lox::bytecode {
enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER };

struct Value {
  Value() = default;

  explicit Value(bool boolean) : type{VAL_BOOL}, boolean{boolean} {}
  explicit Value(double number) : type{VAL_NUMBER}, number{number} {}

  std::string to_string() const;
  bool is_falsey() const;

  ValueType type{VAL_NIL};
  union {
    bool boolean;
    double number{};
  };
};

bool operator==(const Value& left, const Value& right);

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).boolean)
#define AS_NUMBER(value) ((value).number)

using ValueArray = std::vector<Value>;
}  // namespace lox::bytecode
