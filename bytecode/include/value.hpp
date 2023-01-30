#pragma once

#include "common.hpp"

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).boolean)
#define AS_NUMBER(value) ((value).number)
#define AS_OBJ(value) ((value).obj)

namespace lox::bytecode {
struct Obj;

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ };

struct Value {
  Value() = default;

  explicit Value(bool boolean) : type{VAL_BOOL}, boolean{boolean} {}
  explicit Value(double number) : type{VAL_NUMBER}, number{number} {}
  explicit Value(Obj* obj) : type{VAL_OBJ}, obj{obj} {}

  void print() const;
  bool is_falsey() const;

  ValueType type{VAL_NIL};
  union {
    bool boolean;
    double number{};
    Obj* obj;
  };
};

bool operator==(const Value& left, const Value& right);

using ValueArray = std::vector<Value>;
}  // namespace lox::bytecode
