#include "vm.hpp"

#include <iostream>

#include "object.hpp"

namespace lox::bytecode {
void VM::clean_objects() {
  Obj* object = objects;
  while (object != nullptr) {
    Obj* next = object->next;
    delete object;
    object = next;
  }
  objects = nullptr;
}

InterpretResult VM::interpret(const Chunk& chunk) {
  chunk_ = &chunk;
  ip_ = chunk.get_code(0);
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
      std::cout << "[ ";
      slot->print();
      std::cout << " ]";
    }
    std::cout << '\n';
    chunk_->disassemble_instruction(
        static_cast<int>(ip_ - chunk_->get_code(0)));
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
      case OP_POP:
        pop();
        break;
      case OP_GET_GLOBAL: {
        ObjString* name = AS_STRING(read_constant());
        Value value;
        if (!globals.get(name, &value)) {
          runtime_error("Undefined variable '" + name->string + "'.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = AS_STRING(read_constant());
        globals.set(name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = AS_STRING(read_constant());
        if (globals.set(name, peek(0))) {
          globals.del(name);
          runtime_error("Undefined variable '" + name->string + "'.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
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
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          ObjString* b = AS_STRING(pop());
          ObjString* a = AS_STRING(pop());
          push(allocate_object<ObjString>(a->string + b->string));
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          const double b = AS_NUMBER(pop());
          const double a = AS_NUMBER(pop());
          push(Value{a + b});
        } else {
          runtime_error("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
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
      case OP_PRINT:
        pop().print();
        std::cout << '\n';
        break;
      case OP_RETURN:
        return INTERPRET_OK;
      default:
        break;
    }
  }

#undef BINARY_OP
}

void VM::runtime_error(const std::string& message) {
  std::cerr << message << '\n';

  const size_t instruction = ip_ - chunk_->get_code(0) - 1;
  const int line = chunk_->get_line(static_cast<int>(instruction));
  std::cerr << "[line " << line << "] in script" << '\n';
  reset_stack();
}
}  // namespace lox::bytecode
