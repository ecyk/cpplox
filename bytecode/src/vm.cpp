#include "vm.hpp"

#include <iostream>

namespace lox::bytecode {
InterpretResult VM::interpret(const Chunk& chunk) {
  chunk_ = &chunk;
  ip_ = chunk.code();
  reset_stack();
  return run();
}

InterpretResult VM::run() {
#define BINARY_OP(op)       \
  do {                      \
    const double b = pop(); \
    const double a = pop(); \
    push(a op b);           \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (Value* slot = &stack_[0]; slot < stack_top_; slot++) {
      std::cout << "[ " << *slot << " ]";
    }
    std::cout << '\n';
    chunk_->disassemble_instruction(static_cast<int>(ip_ - chunk_->code()));
#endif

    switch (const uint8_t instruction = read_byte()) {
      case OP_CONSTANT:
        push(read_constant());
        break;
      case OP_ADD:
        BINARY_OP(+);
        break;
      case OP_SUBTRACT:
        BINARY_OP(-);
        break;
      case OP_MULTIPLY:
        BINARY_OP(*);
        break;
      case OP_DIVIDE:
        BINARY_OP(/);
        break;
      case OP_NEGATE:
        push(-pop());
        break;
      case OP_RETURN:
        std::cout << pop() << '\n';
        return INTERPRET_OK;
      default:
        break;
    }
  }

#undef BINARY_OP
}
}  // namespace lox::bytecode
