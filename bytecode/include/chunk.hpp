#pragma once

#include "value.hpp"

namespace lox::bytecode {
enum OpCode : uint8_t {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_RETURN
};

class Chunk {
 public:
  void write(uint8_t byte, int line);
  int add_constant(Value value);

  void disassemble(std::string_view name) const;
  int disassemble_instruction(int offset) const;

  [[nodiscard]] const uint8_t* get_code(int offset) const {
    return &code_[offset];
  }
  void set_code(int offset, uint8_t value) { code_[offset] = value; }
  [[nodiscard]] int get_line(int offset) const { return lines_[offset]; }
  [[nodiscard]] Value get_constant(int offset) const {
    return constants_[offset];
  }

  [[nodiscard]] int count() const { return static_cast<int>(code_.size()); }

 private:
  static int simple_instruction(std::string_view name, int offset);
  [[nodiscard]] int constant_instruction(std::string_view name,
                                         int offset) const;
  [[nodiscard]] int byte_instruction(std::string_view name, int offset) const;
  [[nodiscard]] int jump_instruction(std::string_view name, int sign,
                                     int offset) const;

  std::vector<uint8_t> code_;
  std::vector<int> lines_;
  ValueArray constants_;
};
}  // namespace lox::bytecode
