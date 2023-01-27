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
#define BINARY_OP(op)                                 \
  do {                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers.");     \
      return INTERPRET_RUNTIME_ERROR;                 \
    }                                                 \
    const double b = AS_NUMBER(pop());                \
    const double a = AS_NUMBER(pop());                \
    push(Value{a op b});                              \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (const Value* slot = stack_.data(); slot < stack_top_; slot++) {
      std::cout << "[ " << slot->to_string() << " ]";
    }
    std::cout << '\n';
    chunk_->disassemble_instruction(static_cast<int>(ip_ - chunk_->code()));
#endif

    switch (const uint8_t instruction = read_byte()) {
      case OP_CONSTANT:
        push(read_constant());
        break;
      case OP_NIL:
        push(Value{});
        break;
      case OP_TRUE:
        push(Value{true});
        break;
      case OP_FALSE:
        push(Value{false});
        break;
      case OP_EQUAL: {
        const Value b = pop();
        const Value a = pop();
        push(Value{a == b});
        break;
      }
      case OP_GREATER:
        BINARY_OP(>);
        break;
      case OP_LESS:
        BINARY_OP(<);
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
      case OP_NOT:
        push(Value{pop().is_falsey()});
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(Value{-AS_NUMBER(pop())});
        break;
      case OP_RETURN:
        std::cout << pop().to_string() << '\n';
        return INTERPRET_OK;
      default:
        break;
    }
  }

#undef BINARY_OP
}

void VM::runtime_error(const std::string& message) {
  std::cerr << message << '\n';

  const size_t instruction = ip_ - chunk_->code() - 1;
  const int line = chunk_->get_line(static_cast<int>(instruction));
  std::cerr << "[line " << line << "] in script" << '\n';
  reset_stack();
}
}  // namespace lox::bytecode
