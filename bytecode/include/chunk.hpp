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
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_GET_SUPER,
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
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD
};

class Chunk {
 public:
  void write(uint8_t byte, int line);
  size_t add_constant(Value value);

  void disassemble(std::string_view name) const;
  size_t disassemble_instruction(size_t offset) const;

  [[nodiscard]] const std::vector<uint8_t>& get_codes() const { return code_; }
  [[nodiscard]] const std::vector<int>& get_lines() const { return lines_; }
  [[nodiscard]] const ValueArray& get_constants() const { return constants_; }

  void set_code(size_t offset, uint8_t value) { code_[offset] = value; }

 private:
  static size_t simple_instruction(std::string_view name, size_t offset);
  [[nodiscard]] size_t constant_instruction(std::string_view name,
                                            size_t offset) const;
  [[nodiscard]] size_t byte_instruction(std::string_view name,
                                        size_t offset) const;
  [[nodiscard]] size_t jump_instruction(std::string_view name, int sign,
                                        size_t offset) const;
  [[nodiscard]] size_t invoke_instruction(std::string_view name,
                                          size_t offset) const;

  std::vector<uint8_t> code_;
  std::vector<int> lines_;
  ValueArray constants_;
};
}  // namespace lox::bytecode
