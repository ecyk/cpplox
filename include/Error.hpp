#pragma once

#include <stdexcept>
#include <string>

class LoxError : public std::runtime_error {
 public:
  LoxError(size_t line, const std::string& message);
};
