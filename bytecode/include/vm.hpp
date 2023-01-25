#pragma once

#include "chunk.hpp"

#define STACK_MAX 256

namespace lox::bytecode {
enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
 public:
  VM() { reset_stack(); }

  InterpretResult interpret(const Chunk& chunk);

 private:
  InterpretResult run();

  void push(Value value) { *stack_top_++ = value; }
  Value pop() { return *--stack_top_; }

  void reset_stack() {
    std::fill(std::begin(stack_), std::end(stack_), 0);
    stack_top_ = &stack_[0];
  }

  uint8_t read_byte() { return *ip_++; }
  Value read_constant() { return chunk_->get_constant(read_byte()); }

  const Chunk* chunk_{};
  const uint8_t* ip_{};
  Value stack_[STACK_MAX];
  Value* stack_top_;
};
}  // namespace lox::bytecode
