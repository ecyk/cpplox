#pragma once

#include <array>

#include "object.hpp"
#include "table.hpp"

namespace lox::bytecode {
enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
  struct CallFrame {
    ObjClosure* closure{};
    const uint8_t* ip{};
    Value* slots{};
  };

  static constexpr int FRAMES_MAX = 64;
  static constexpr int STACK_MAX = FRAMES_MAX * UINT8_COUNT;

  inline static Obj* objects{};
  inline static ObjUpvalue* open_upvalues{};
  inline static Table globals;
  inline static Table strings;

 public:
  template <typename ObjT, typename... Args>
  static ObjT* allocate_object(Args&&... args) {
    ObjT* object{};
    if constexpr (std::is_same_v<ObjT, ObjString>) {
      const uint32_t hash = ::hash({std::forward<Args>(args)...});
      ObjString* interned =
          strings.find_string({std::forward<Args>(args)...}, hash);

      if (interned != nullptr) {
        return interned;
      }

      object = new ObjString{std::string{std::forward<Args>(args)...}, hash};
    } else {
      object = new ObjT{std::forward<Args>(args)...};
    }

    object->next_object = objects;
    objects = object;

    if constexpr (std::is_same_v<ObjT, ObjString>) {
      strings.set(object, {});
    }

    return object;
  }

  static void clean_objects();

  VM();

  InterpretResult interpret(ObjFunction* function);

  void define_native(std::string_view name, NativeFn function);

 private:
  InterpretResult run();

  bool call_value(Value callee, int arg_count);
  bool call(ObjClosure* closure, int arg_count);

  static ObjUpvalue* capture_upvalue(Value* local);
  static void close_upvalues(Value* last);

  void reset_stack();
  void push(Value value) { *stack_top_++ = value; }
  Value pop() { return *--stack_top_; }
  Value peek(int distance) { return *(stack_top_ - 1 - distance); }
  uint8_t read_byte() { return *frame_top_->ip++; }
  uint16_t read_short() {
    return (frame_top_->ip += 2,
            static_cast<uint16_t>(*(frame_top_->ip - 2) << 8U) |
                *(frame_top_->ip - 1));
  }
  Value read_constant() {
    return frame_top_->closure->function->chunk.get_constant(read_byte());
  }

  void runtime_error(const std::string& message);

  std::array<CallFrame, FRAMES_MAX> frames_;
  CallFrame* frame_top_{};
  int frame_count_{};

  std::array<Value, STACK_MAX> stack_;
  Value* stack_top_{};
};
}  // namespace lox::bytecode
