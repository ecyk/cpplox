#pragma once

#include "value.hpp"

namespace lox::bytecode {
enum OpCode : uint8_t {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_RETURN
};

class Chunk {
 public:
  void write(OpCode byte, int line);
  int add_constant(Value value);
  Value get_constant(int index) const;

  void disassemble(const std::string& name) const;
  int disassemble_instruction(int offset) const;

  const uint8_t* code() const { return code_.data(); }

 private:
  int simple_instruction(const char* name, int offset) const;
  int constant_instruction(const char* name, int offset) const;

  std::vector<uint8_t> code_;
  std::vector<int> lines_;
  ValueArray constants_;
};
}  // namespace lox::bytecode
