#include "value.hpp"

#include <iostream>

#include "stmt.hpp"

namespace lox::treewalk {
void print_value(const Value& value) {
  if (IS_NIL(value)) {
    std::cout << "nil";
    return;
  }
  if (IS_STRING(value)) {
    std::cout << AS_STRING(value);
    return;
  }
  if (IS_NUMBER(value)) {
    std::string number = std::to_string(AS_NUMBER(value));
    number.erase(number.find_last_not_of('0') + 1, std::string::npos);
    number.erase(number.find_last_not_of('.') + 1, std::string::npos);
    std::cout << number;
    return;
  }
  if (IS_BOOL(value)) {
    std::cout << (AS_BOOL(value) ? "true" : "false");
    return;
  }
  if (IS_FUNCTION(value)) {
    std::cout << "<fn " << AS_FUNCTION(value)->declaration->name.lexeme << ">";
    return;
  }
  if (IS_NATIVE(value)) {
    std::cout << "<native fn>";
    return;
  }
  if (IS_CLASS(value)) {
    std::cout << AS_CLASS(value)->declaration->name.lexeme;
    return;
  }
  if (IS_INSTANCE(value)) {
    std::cout << AS_INSTANCE(value)->class_->declaration->name.lexeme
              << " instance";
    return;
  }
}
}  // namespace lox::treewalk
