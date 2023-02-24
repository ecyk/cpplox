#pragma once

#include <array>

#include "chunk.hpp"
#include "table.hpp"

namespace lox::bytecode {
enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
  static constexpr size_t STACK_MAX = 256;

  inline static Obj* objects{};
  inline static Table strings;

 public:
  VM() = default;
  ~VM() { clean_objects(); }

  VM(const VM&) = delete;
  VM& operator=(const VM&) = delete;
  VM(VM&&) = delete;
  VM& operator=(VM&&) = delete;

  InterpretResult interpret(const Chunk& chunk);

  template <typename ObjT, typename... Args>
  static Value allocate_object(Args&&... args) {
    if constexpr (std::is_same_v<ObjT, ObjString>) {
      ObjString* interned = strings.find_string(std::forward<Args>(args)...);

      if (interned != nullptr) {
        return Value{interned};
      }
    }

    ObjT* object = new ObjT{std::forward<Args>(args)...};

    object->next = objects;
    objects = object;

    if constexpr (std::is_same_v<ObjT, ObjString>) {
      strings.set(*object, {});
    }

    return Value{object};
  }

  static void clean_objects();

 private:
  InterpretResult run();

  void push(Value value) { *stack_top_++ = value; }
  Value pop() { return *--stack_top_; }

  Value peek(int distance) { return *(stack_top_ - 1 - distance); }

  void reset_stack() {
    stack_.fill({});
    stack_top_ = stack_.data();
  }

  uint8_t read_byte() { return *ip_++; }
  Value read_constant() { return chunk_->get_constant(read_byte()); }

  void runtime_error(const std::string& message);

  const Chunk* chunk_{};
  const uint8_t* ip_{};

  std::array<Value, STACK_MAX> stack_;
  Value* stack_top_{};
};
}  // namespace lox::bytecode
