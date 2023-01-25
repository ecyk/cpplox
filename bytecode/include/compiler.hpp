#pragma once

#include <string>

namespace lox::bytecode {
class Compiler {
 public:
  void compile(const std::string& source);
};
}  // namespace lox::bytecode
