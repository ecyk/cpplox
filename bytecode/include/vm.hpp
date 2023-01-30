#pragma once

#include <array>

#include "chunk.hpp"

#define STACK_MAX 256

namespace lox::bytecode {
enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
  inline static Obj* objects{};

 public:
  ~VM() { clean_objects(); }

  InterpretResult interpret(const Chunk& chunk);

  template <typename ObjT, typename... Args>
  static ObjT* allocate_object(Args&&... args) {
    ObjT* object = new ObjT{std::forward<Args>(args)...};
    object->next = objects;
    objects = object;
    return object;
  }

  static void clean_objects();

 private:
  InterpretResult run();

  void push(Value value) { *stack_top_++ = value; }
  Value pop() { return *--stack_top_; }

  Value peek(int distance) { return *(stack_top_ - 1 - distance); }

  void reset_stack() {
    stack_.fill({});
    stack_top_ = &stack_[0];
  }

  uint8_t read_byte() { return *ip_++; }
  Value read_constant() { return chunk_->get_constant(read_byte()); }

  void runtime_error(const std::string& message);

  const Chunk* chunk_{};
  const uint8_t* ip_{};

  std::array<Value, STACK_MAX> stack_;
  Value* stack_top_;
};
}  // namespace lox::bytecode
