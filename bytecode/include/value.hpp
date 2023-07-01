#pragma once

#include <cstring>

#include "common.hpp"

#ifdef NAN_BOXING

#define SIGN_BIT (0x8000000000000000U)
#define QNAN (0x7ffc000000000000U)

#define TAG_NIL 1U
#define TAG_FALSE 2U
#define TAG_TRUE 3U

#define BOOL_VAL(bool) ((bool) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL (QNAN | TAG_FALSE)
#define TRUE_VAL (QNAN | TAG_TRUE)
#define NIL_VAL (QNAN | TAG_NIL)
#define NUMBER_VAL(number) (number_to_value(number))
#define OBJ_VAL(obj) (SIGN_BIT | QNAN | reinterpret_cast<uintptr_t>(obj))

#define IS_BOOL(value) (((value) | 1U) == TRUE_VAL)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_NUMBER(value) (((value)&QNAN) != QNAN)
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value) ((value) == TRUE_VAL)
#define AS_NUMBER(value) (value_to_number(value))
#define AS_OBJ(value) (reinterpret_cast<Obj*>((value) & ~(SIGN_BIT | QNAN)))

#else

#define BOOL_VAL(bool) ((bool) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL (Value{false})
#define TRUE_VAL (Value{true})
#define NIL_VAL (Value{})
#define NUMBER_VAL(number) (Value{number})
#define OBJ_VAL(obj) (Value{obj})

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).boolean)
#define AS_NUMBER(value) ((value).number)
#define AS_OBJ(value) ((value).obj)

#endif

namespace lox::bytecode {
#ifdef NAN_BOXING

using Value = uint64_t;

static inline Value number_to_value(double number) {
  Value value{};
  memcpy(&value, &number, sizeof(double));
  return value;
}

static inline double value_to_number(Value value) {
  double number{};
  memcpy(&number, &value, sizeof(Value));
  return number;
}

#else
struct Obj;

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ };

struct Value {
  Value() = default;

  explicit Value(bool boolean) : type{VAL_BOOL}, boolean{boolean} {}
  explicit Value(double number) : type{VAL_NUMBER}, number{number} {}
  explicit Value(Obj* obj) : type{VAL_OBJ}, obj{obj} {}

  ValueType type{VAL_NIL};
  union {
    bool boolean;
    double number{};
    Obj* obj;
  };
};

#endif

void print_value(Value value);
bool values_equal(Value left, Value right);

static inline bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

using ValueArray = std::vector<Value>;
}  // namespace lox::bytecode
