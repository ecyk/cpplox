#include "value.hpp"

namespace lox::bytecode {
std::string Value::to_string() const {
  switch (type) {
    case VAL_BOOL:
      return AS_BOOL(*this) ? "true" : "false";
    case VAL_NIL:
      return "nil";
    case VAL_NUMBER: {
      std::string number = std::to_string(AS_NUMBER(*this));
      number.erase(number.find_last_not_of('0') + 1, std::string::npos);
      number.erase(number.find_last_not_of('.') + 1, std::string::npos);
      return number;
    }
    default:
      return "";
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
    default:
      return false;
  }
}
}  // namespace lox::bytecode
