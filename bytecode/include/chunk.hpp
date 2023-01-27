#pragma once

#include "value.hpp"

namespace lox::bytecode {
enum OpCode : uint8_t {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_RETURN
};

class Chunk {
 public:
  void write(uint8_t byte, int line);
  int add_constant(Value value);
  Value get_constant(int offset) const;

  void disassemble(const std::string& name) const;
  int disassemble_instruction(int offset) const;

  int get_line(int offset) const { return lines_[offset]; }
  const uint8_t* code() const { return code_.data(); }

 private:
  int simple_instruction(const char* name, int offset) const;
  int constant_instruction(const char* name, int offset) const;

  std::vector<uint8_t> code_;
  std::vector<int> lines_;
  ValueArray constants_;
};
}  // namespace lox::bytecode
