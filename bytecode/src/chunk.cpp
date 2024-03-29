#include "chunk.hpp"

#include <iomanip>
#include <iostream>

#include "object.hpp"
#include "vm.hpp"

namespace lox::bytecode {
void Chunk::write(uint8_t byte, int line) {
  code_.push_back(byte);
  lines_.push_back(line);
}

size_t Chunk::add_constant(Value value) {
  g_vm.push(value);
  constants_.push_back(value);
  g_vm.pop();
  return constants_.size() - 1;
}

void Chunk::disassemble(std::string_view name) const {
  std::cout << "== " << name << " ==\n";

  for (size_t offset = 0; offset < code_.size();) {
    offset = disassemble_instruction(offset);
  }
}

size_t Chunk::disassemble_instruction(size_t offset) const {
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
    case OP_GET_PROPERTY:
      return constant_instruction("OP_GET_PROPERTY", offset);
    case OP_SET_PROPERTY:
      return constant_instruction("OP_SET_PROPERTY", offset);
    case OP_GET_SUPER:
      return constant_instruction("OP_GET_SUPER", offset);
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
    case OP_INVOKE:
      return invoke_instruction("OP_INVOKE", offset);
    case OP_SUPER_INVOKE:
      return invoke_instruction("OP_SUPER_INVOKE", offset);
    case OP_CLOSURE: {
      offset++;
      const uint32_t constant = code_[offset++];
      std::cout << std::setfill(' ') << std::setw(16) << std::left
                << "OP_CLOSURE" << std::setw(4) << std::right << constant
                << ' ';
      print_value(constants_[constant]);
      std::cout << '\n';

      ObjFunction* function = AS_FUNCTION(constants_[constant]);
      for (int i = 0; i < function->upvalue_count; i++) {
        const uint32_t is_local = code_[offset++];
        const uint32_t index = code_[offset++];
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
    case OP_CLASS:
      return constant_instruction("OP_CLASS", offset);
    case OP_INHERIT:
      return simple_instruction("OP_INHERIT", offset);
    case OP_METHOD:
      return constant_instruction("OP_METHOD", offset);
    default:
      std::cout << "Unknown opcode " << instruction << '\n';
      return offset + 1;
  }
}

size_t Chunk::simple_instruction(std::string_view name, size_t offset) {
  std::cout << name << '\n';
  return offset + 1;
}

size_t Chunk::constant_instruction(std::string_view name, size_t offset) const {
  const uint32_t constant = code_[offset + 1];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name
            << std::setw(4) << std::right << constant << " '";
  print_value(constants_[constant]);
  std::cout << "'\n";
  return offset + 2;
}

size_t Chunk::byte_instruction(std::string_view name, size_t offset) const {
  const uint32_t slot = code_[offset + 1];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name
            << std::setw(4) << std::right << slot << '\n';
  return offset + 2;
}

size_t Chunk::jump_instruction(std::string_view name, int sign,
                               size_t offset) const {
  auto jump = static_cast<uint32_t>(code_[offset + 1] << 8U);
  jump |= code_[offset + 2];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name
            << std::setw(4) << std::right << offset << " -> "
            << static_cast<int>(offset) + 3 + sign * static_cast<int>(jump)
            << '\n';
  return offset + 3;
}

size_t Chunk::invoke_instruction(std::string_view name, size_t offset) const {
  const uint32_t constant = code_[offset + 1];
  const uint32_t arg_count = code_[offset + 2];
  std::cout << std::setfill(' ') << std::setw(16) << std::left << name
            << std::setw(4) << std::right << '(' << arg_count << " args) "
            << std::setfill('0') << std::setw(4) << std::right << constant
            << " '";
  print_value(constants_[constant]);
  std::cout << "'\n";
  return offset + 3;
}
}  // namespace lox::bytecode
