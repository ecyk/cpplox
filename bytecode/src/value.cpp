#include "value.hpp"

#include <iostream>
#include <limits>

#include "object.hpp"

namespace lox::bytecode {
void Value::print() const {
  switch (type) {
    case VAL_BOOL:
      std::cout << (AS_BOOL(*this) ? "true" : "false");
      break;
    case VAL_NIL:
      std::cout << "nil";
      break;
    case VAL_NUMBER:
      std::cout << AS_NUMBER(*this);
      break;
    case VAL_OBJ:
      switch (OBJ_TYPE(*this)) {
        case OBJ_STRING:
          std::cout << AS_STRING(*this)->string;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

bool Value::is_falsey() const {
  return IS_NIL(*this) || (IS_BOOL(*this) && !AS_BOOL(*this));
}

bool operator==(const Value& left, const Value& right) {
  if (left.type != right.type) {
    return false;
  }

  switch (left.type) {
    case VAL_BOOL:
      return AS_BOOL(left) == AS_BOOL(right);
    case VAL_NIL:
      return true;
    case VAL_NUMBER:
      return std::abs(AS_NUMBER(left) - AS_NUMBER(right)) <=
             std::max(std::abs(AS_NUMBER(left)), std::abs(AS_NUMBER(right))) *
                 std::numeric_limits<double>::epsilon();
    case VAL_OBJ:
      return AS_OBJ(left) == AS_OBJ(right);
    default:
      return false;
  }
}
}  // namespace lox::bytecode
