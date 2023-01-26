#include "chunk.hpp"

#include <iomanip>
#include <iostream>

namespace lox::bytecode {
void Chunk::write(uint8_t byte, int line) {
  code_.push_back(byte);
  lines_.push_back(line);
}

int Chunk::add_constant(Value value) {
  constants_.push_back(value);
  return static_cast<int>(constants_.size()) - 1;
}

Value Chunk::get_constant(int index) const { return constants_[index]; }

void Chunk::disassemble(const std::string& name) const {
  std::cout << "== " << name << " ==\n";

  for (int offset = 0; offset < code_.size();) {
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
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    default:
      std::cout << "Unknown opcode " << instruction << '\n';
      return offset + 1;
  }
}

int Chunk::simple_instruction(const char* name, int offset) const {
  std::cout << name << '\n';
  return offset + 1;
}

int Chunk::constant_instruction(const char* name, int offset) const {
  const int constant = code_[offset + 1];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name << ' '
            << std::setw(4) << std::right << constant << " '";
  std::cout << constants_[constant] << "'\n";
  return offset + 2;
}
}  // namespace lox::bytecode
