#include "vm.hpp"

#include <chrono>
#include <iostream>

namespace lox::bytecode {
Value clock_native(int /*arg_count*/, Value* /*args*/) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

  constexpr double ms_to_seconds = 1.0 / 1000;
  return Value{static_cast<double>(ms) * ms_to_seconds};
}

void VM::clean_objects() {
  Obj* object = objects;
  while (object != nullptr) {
    Obj* next = object->next;
    delete object;
    object = next;
  }
  objects = nullptr;
}

VM::VM() {
  reset_stack();
  define_native("clock", clock_native);
}

InterpretResult VM::interpret(ObjFunction* function) {
  reset_stack();

  push(Value{function});
  call(function, 0);

  return run();
}

void VM::define_native(std::string_view name, NativeFn function) {
  push(Value{allocate_object<ObjString>(name)});
  push(Value{allocate_object<ObjNative>(function)});
  globals.set(AS_STRING(stack_[0]), stack_[1]);
  pop();
  pop();
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

  frame_top_ = &frames_[frame_count_ - 1];

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (const Value* slot = stack_.data(); slot < stack_top_; slot++) {
      std::cout << "[ ";
      slot->print();
      std::cout << " ]";
    }
    std::cout << '\n';
    frame_top_->function->chunk.disassemble_instruction(static_cast<int>(
        frame_top_->ip - frame_top_->function->chunk.get_code(0)));
#endif

    switch (const uint8_t instruction = read_byte()) {
      case OP_CONSTANT:
        push(read_constant());
        break;
      case OP_NIL:
        push({});
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
      case OP_GET_LOCAL: {
        const uint8_t slot = read_byte();
        push(frame_top_->slots[slot]);  // problem is here +1 acceptable
        break;
      }
      case OP_SET_LOCAL: {
        const uint8_t slot = read_byte();
        frame_top_->slots[slot] = peek(0);
        break;
      }
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
      case OP_JUMP: {
        const uint16_t offset = read_short();
        frame_top_->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        const uint16_t offset = read_short();
        if (peek(0).is_falsey()) {
          frame_top_->ip += offset;
        }
        break;
      }
      case OP_LOOP: {
        const uint16_t offset = read_short();
        frame_top_->ip -= offset;
        break;
      }
      case OP_CALL: {
        const int arg_count = read_byte();
        if (!call_value(peek(arg_count), arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame_top_ = &frames_[frame_count_ - 1];
        break;
      }
      case OP_RETURN: {
        const Value result = pop();
        if (--frame_count_ == 0) {
          pop();
          return INTERPRET_OK;
        }
        stack_top_ = frame_top_->slots;
        push(result);
        frame_top_ = &frames_[frame_count_ - 1];
        break;
      }
      default:
        break;
    }
  }

#undef BINARY_OP
}

bool VM::call_value(Value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_FUNCTION:
        return call(AS_FUNCTION(callee), arg_count);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        const Value result = native(arg_count, stack_top_ - arg_count);
        stack_top_ -= arg_count + 1;
        push(result);
        return true;
      }
      default:
        break;
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

bool VM::call(ObjFunction* function, int arg_count) {
  if (arg_count != function->arity) {
    runtime_error("Expected " + std::to_string(function->arity) +
                  " arguments but got " + std::to_string(arg_count) + ".");
    return false;
  }

  if (frame_count_ == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame* frame = &frames_[frame_count_++];
  frame->function = function;
  frame->ip = function->chunk.get_code(0);
  frame->slots = stack_top_ - arg_count - 1;
  return true;
}

void VM::reset_stack() {
  stack_.fill({});
  stack_top_ = stack_.data();
}

void VM::runtime_error(const std::string& message) {
  std::cerr << message << '\n';

  for (int i = frame_count_ - 1; i >= 0; i--) {
    CallFrame* frame = &frames_[i];
    ObjFunction* function = frame->function;
    const int instruction = frame->ip - function->chunk.get_code(0) - 1;
    std::cerr << "[line " << function->chunk.get_line(instruction) << "] in ";
    if (function->name == nullptr) {
      std::cerr << "script\n";
    } else {
      std::cerr << function->name->string << "()\n";
    }
  }

  reset_stack();
}
}  // namespace lox::bytecode
