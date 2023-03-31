#include "chunk.hpp"

#include <iomanip>
#include <iostream>

#include "object.hpp"

namespace lox::bytecode {
void Chunk::write(uint8_t byte, int line) {
  code_.push_back(byte);
  lines_.push_back(line);
}

int Chunk::add_constant(Value value) {
  constants_.push_back(value);
  return static_cast<int>(constants_.size()) - 1;
}

void Chunk::disassemble(std::string_view name) const {
  std::cout << "== " << name << " ==\n";

  for (int offset = 0; offset < static_cast<int>(code_.size());) {
    offset = disassemble_instruction(offset);
  }
}

int Chunk::disassemble_instruction(int offset) const {
  std::cout << std::setfill('0') << std::setw(4) << offset << ' ';
  if (offset > 0 && lines_[offset] == lines_[offset - 1]) {
    std::cout << "   | ";
  } else {
    std::cout << std::setfill(' ') << std::setw(4) << lines_[offset] << ' ';
  }

  const uint8_t instruction = code_[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constant_instruction("OP_CONSTANT", offset);
    case OP_NIL:
      return simple_instruction("OP_NIL", offset);
    case OP_TRUE:
      return simple_instruction("OP_TRUE", offset);
    case OP_FALSE:
      return simple_instruction("OP_FALSE", offset);
    case OP_POP:
      return simple_instruction("OP_POP", offset);
    case OP_GET_LOCAL:
      return byte_instruction("OP_GET_LOCAL", offset);
    case OP_SET_LOCAL:
      return byte_instruction("OP_SET_LOCAL", offset);
    case OP_GET_GLOBAL:
      return constant_instruction("OP_GET_GLOBAL", offset);
    case OP_DEFINE_GLOBAL:
      return constant_instruction("OP_DEFINE_GLOBAL", offset);
    case OP_SET_GLOBAL:
      return constant_instruction("OP_SET_GLOBAL", offset);
    case OP_GET_UPVALUE:
      return byte_instruction("OP_GET_UPVALUE", offset);
    case OP_SET_UPVALUE:
      return byte_instruction("OP_SET_UPVALUE", offset);
    case OP_EQUAL:
      return simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simple_instruction("OP_GREATER", offset);
    case OP_LESS:
      return simple_instruction("OP_LESS", offset);
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_NOT:
      return simple_instruction("OP_NOT", offset);
    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_PRINT:
      return simple_instruction("OP_PRINT", offset);
    case OP_JUMP:
      return jump_instruction("OP_JUMP", 1, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instruction("OP_JUMP_IF_FALSE", 1, offset);
    case OP_LOOP:
      return jump_instruction("OP_LOOP", -1, offset);
    case OP_CALL:
      return byte_instruction("OP_CALL", offset);
    case OP_CLOSURE: {
      offset++;
      const int constant = code_[offset++];
      std::cout << std::setfill(' ') << std::setw(16) << std::left
                << "OP_CLOSURE" << ' ' << std::setw(4) << std::right << constant
                << ' ';
      constants_[constant].print();
      std::cout << '\n';

      ObjFunction* function = AS_FUNCTION(constants_[constant]);
      for (int j = 0; j < function->upvalue_count; j++) {
        const int is_local = code_[offset++];
        const int index = code_[offset++];
        std::cout << std::setfill('0') << std::setw(4) << std::right
                  << offset - 2 << "      |                     "
                  << (is_local != 0 ? "local" : "upvalue") << ' ' << index
                  << '\n';
      }

      return offset;
    }
    case OP_CLOSE_UPVALUE:
      return simple_instruction("OP_CLOSE_UPVALUE", offset);
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    default:
      std::cout << "Unknown opcode " << instruction << '\n';
      return offset + 1;
  }
}

int Chunk::simple_instruction(std::string_view name, int offset) {
  std::cout << name << '\n';
  return offset + 1;
}

int Chunk::constant_instruction(std::string_view name, int offset) const {
  const int constant = code_[offset + 1];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name << ' '
            << std::setw(4) << std::right << constant << " '";
  constants_[constant].print();
  std::cout << "'\n";
  return offset + 2;
}

int Chunk::byte_instruction(std::string_view name, int offset) const {
  const int slot = code_[offset + 1];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name << ' '
            << std::setw(4) << std::right << slot << '\n';
  return offset + 2;
}

int Chunk::jump_instruction(std::string_view name, int sign, int offset) const {
  uint16_t jump = code_[offset + 1] << 8U;
  jump |= code_[offset + 2];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name
            << std::setw(4) << std::right << ' ' << offset << " -> "
            << offset + 3 + sign * jump << '\n';
  return offset + 3;
}
}  // namespace lox::bytecode
