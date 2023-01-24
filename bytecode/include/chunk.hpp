#pragma once

#include "value.hpp"

namespace lox::bytecode {
enum OpCode : uint8_t { OP_CONSTANT, OP_RETURN };

class Chunk {
 public:
  void write(uint8_t byte, int line);
  int add_constant(Value value);

  void disassemble(const std::string& name) const;
  [[nodiscard]] int disassemble_instruction(int offset) const;

 private:
  int simple_instruction(const char* name, int offset) const;
  int constant_instruction(const char* name, int offset) const;

  std::vector<uint8_t> code_;
  std::vector<int> lines_;
  ValueArray constants_;
};
}  // namespace lox::bytecode
