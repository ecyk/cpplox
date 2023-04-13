#pragma once

#include <array>
#include <cstddef>
#include <iostream>
#include <stack>

#include "compiler.hpp"
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

  static constexpr size_t GC_HEAP_GROW_FACTOR = 2;

 public:
  VM();

  InterpretResult interpret(ObjFunction* function);

  void define_native(std::string_view name, NativeFn function);

 private:
  InterpretResult run();

  void define_method(ObjString* name);
  bool bind_method(ObjClass* class_, ObjString* name);
  bool invoke_from_class(ObjClass* class_, ObjString* name, int arg_count);
  bool invoke(ObjString* name, int arg_count);
  bool call_value(Value callee, int arg_count);
  bool call(ObjClosure* closure, int arg_count);

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
    return frame_top_->closure->function->chunk.get_constants()[read_byte()];
  }

  ObjUpvalue* capture_upvalue(Value* local);
  void close_upvalues(const Value* last);

  void runtime_error(const std::string& message);

  Obj* objects_{};
  Table globals_;
  Table strings_;
  ObjString* init_string_{};

  std::array<CallFrame, FRAMES_MAX> frames_;
  CallFrame* frame_top_{};
  int frame_count_{};

  std::array<Value, STACK_MAX> stack_{};
  Value* stack_top_{};

  ObjUpvalue* open_upvalues_{};

 public:
  template <typename ObjT, typename... Args>
  ObjT* allocate_object(Args&&... args) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif

    if (bytes_allocated_ > next_gc_) {
      collect_garbage();
    }

    ObjT* object{};
    if constexpr (std::is_same_v<ObjT, ObjString>) {
      const uint32_t hash = ::hash({std::forward<Args>(args)...});
      ObjString* interned =
          strings_.find_string({std::forward<Args>(args)...}, hash);

      if (interned != nullptr) {
        return interned;
      }

      object = new ObjString{std::string{std::forward<Args>(args)...}, hash};
    } else {
      object = new ObjT{std::forward<Args>(args)...};
    }

    object->next_object = objects_;
    objects_ = object;

    if constexpr (std::is_same_v<ObjT, ObjString>) {
      push(OBJ_VAL(object));
      strings_.set(object, NIL_VAL);
      pop();
    }

    bytes_allocated_ += sizeof(ObjT);

#ifdef DEBUG_LOG_GC
    std::cout << (void*)object << " allocate " << sizeof(ObjT) << " for "
              << object->type << "\n";
#endif

    return object;
  }

  void collect_garbage();
  void free_objects();

 private:
  void mark_object(Obj* object);
  void mark_value(Value value);
  void mark_table(const Table& table);
  void blacken_object(Obj* object);

  void mark_roots();
  void trace_references();
  void sweep();

  size_t bytes_allocated_{};
  size_t next_gc_{static_cast<size_t>(1024 * 1024)};
  std::stack<Obj*> gray_stack_;

  friend int Chunk::add_constant(Value value);
  friend void Compiler::mark_compiler_roots();
};

inline VM g_vm;
}  // namespace lox::bytecode
