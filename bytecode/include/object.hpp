#pragma once

#include "chunk.hpp"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_CLOSURE(value) (is_obj_type(value, OBJ_CLOSURE))
#define IS_FUNCTION(value) (is_obj_type(value, OBJ_FUNCTION))
#define IS_NATIVE(value) (is_obj_type(value, OBJ_NATIVE))
#define IS_STRING(value) (is_obj_type(value, OBJ_STRING))

#define AS_CLOSURE(value) (static_cast<ObjClosure*>(AS_OBJ(value)))
#define AS_FUNCTION(value) (static_cast<ObjFunction*>(AS_OBJ(value)))
#define AS_NATIVE(value) (static_cast<ObjNative*>(AS_OBJ(value))->function)
#define AS_STRING(value) (static_cast<ObjString*>(AS_OBJ(value)))

namespace lox::bytecode {
struct ObjString;

enum ObjType { OBJ_CLOSURE, OBJ_FUNCTION, OBJ_NATIVE, OBJ_STRING, OBJ_UPVALUE };

struct Obj {
  virtual ~Obj() = default;

  explicit Obj(ObjType type) : type{type} {}

  Obj(const Obj&) = delete;
  Obj& operator=(const Obj&) = delete;
  Obj(Obj&&) = delete;
  Obj& operator=(Obj&&) = delete;

  ObjType type;
  bool is_marked{};
  Obj* next_object{};
};

struct ObjFunction : Obj {
  ObjFunction() : Obj{OBJ_FUNCTION} {}

  int arity{};
  int upvalue_count{};
  Chunk chunk;
  ObjString* name{};
};

using NativeFn = Value (*)(int arg_count, Value* args);

struct ObjNative : Obj {
  explicit ObjNative(NativeFn function) : Obj{OBJ_NATIVE}, function{function} {}

  NativeFn function;
};

struct ObjString : Obj {
  ObjString(std::string string, uint32_t hash)
      : Obj{OBJ_STRING}, string{std::move(string)}, hash{hash} {}

  std::string string;
  uint32_t hash;
};

struct ObjUpvalue : Obj {
  explicit ObjUpvalue(Value* location) : Obj{OBJ_UPVALUE}, location{location} {}

  Value* location;
  Value closed;
  ObjUpvalue* next_upvalue{};
};

struct ObjClosure : Obj {
  explicit ObjClosure(ObjFunction* function)
      : Obj{OBJ_CLOSURE},
        function{function},
        upvalue_count{function->upvalue_count} {
    for (int i = 0; i < upvalue_count; i++) {
      upvalues.push_back(nullptr);
    }
  }

  ObjFunction* function;
  std::vector<ObjUpvalue*> upvalues;
  int upvalue_count;
};

inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
}  // namespace lox::bytecode
