#include "value.hpp"

#include <iostream>
#include <limits>

#include "object.hpp"

namespace lox::bytecode {
namespace {
void print_function(ObjFunction* function) {
  if (function->name == nullptr) {
    std::cout << "<script>";
    return;
  }
  std::cout << "<fn " << function->name->string << ">";
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
      print_function(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CLASS:
      std::cout << AS_CLASS(value)->name->string;
      break;
    case OBJ_CLOSURE:
      print_function(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      print_function(AS_FUNCTION(value));
      break;
    case OBJ_INSTANCE:
      std::cout << AS_INSTANCE(value)->class_->name->string << " instance";
      break;
    case OBJ_NATIVE:
      std::cout << "<native fn>";
      break;
    case OBJ_STRING:
      std::cout << AS_STRING(value)->string;
      break;
    case OBJ_UPVALUE:
      std::cout << "upvalue";
      break;
    default:
      break;
  }
}
}  // namespace

void print_value(Value value) {
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    std::cout << (AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    std::cout << "nil";
  } else if (IS_NUMBER(value)) {
    std::string number = std::to_string(AS_NUMBER(value));
    number.erase(number.find_last_not_of('0') + 1, std::string::npos);
    number.erase(number.find_last_not_of('.') + 1, std::string::npos);
    std::cout << number;
  } else if (IS_OBJ(value)) {
    print_object(value);
  }
#else
  switch (value.type) {
    case VAL_BOOL:
      std::cout << (AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL:
      std::cout << "nil";
      break;
    case VAL_NUMBER: {
      std::string number = std::to_string(AS_NUMBER(value));
      number.erase(number.find_last_not_of('0') + 1, std::string::npos);
      number.erase(number.find_last_not_of('.') + 1, std::string::npos);
      std::cout << number;
      break;
    }
    case VAL_OBJ:
      print_object(value);
      break;
    default:
      break;
  }
#endif
}

bool values_equal(Value left, Value right) {
#ifdef NAN_BOXING
  if (IS_NUMBER(left) && IS_NUMBER(right)) {
    return std::abs(AS_NUMBER(left) - AS_NUMBER(right)) <=
           std::max(std::abs(AS_NUMBER(left)), std::abs(AS_NUMBER(right))) *
               std::numeric_limits<double>::epsilon();
  }
  return left == right;
#else
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
#endif
}
}  // namespace lox::bytecode
