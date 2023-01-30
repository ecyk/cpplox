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

  void disassemble(std::string_view name) const;
  int disassemble_instruction(int offset) const;

  const uint8_t& get_code(int offset) const { return code_[offset]; }
  const int& get_line(int offset) const { return lines_[offset]; }
  const Value& get_constant(int offset) const { return constants_[offset]; }

 private:
  static int simple_instruction(std::string_view name, int offset);
  int constant_instruction(std::string_view name, int offset) const;

  std::vector<uint8_t> code_;
  std::vector<int> lines_;
  ValueArray constants_;
};
}  // namespace lox::bytecode
