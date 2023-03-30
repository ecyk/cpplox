#pragma once

#include "chunk.hpp"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) (is_obj_type(value, OBJ_FUNCTION))
#define IS_NATIVE(value) (is_obj_type(value, OBJ_NATIVE))
#define IS_STRING(value) (is_obj_type(value, OBJ_STRING))

#define AS_FUNCTION(value) (static_cast<ObjFunction*>(AS_OBJ(value)))
#define AS_NATIVE(value) (static_cast<ObjNative*>(AS_OBJ(value))->function)
#define AS_STRING(value) (static_cast<ObjString*>(AS_OBJ(value)))

namespace lox::bytecode {
enum ObjType { OBJ_FUNCTION, OBJ_NATIVE, OBJ_STRING };

struct Obj {
  virtual ~Obj() = default;

  explicit Obj(ObjType type) : type{type} {}

  Obj(const Obj&) = delete;
  Obj& operator=(const Obj&) = delete;
  Obj(Obj&&) = delete;
  Obj& operator=(Obj&&) = delete;

  ObjType type;
  Obj* next{};
};

struct ObjString : Obj {
  ObjString(std::string string, uint32_t hash)
      : Obj{OBJ_STRING}, string{std::move(string)}, hash{hash} {}
  ObjString(std::string_view string, uint32_t hash)
      : Obj{OBJ_STRING}, string{string}, hash{hash} {}

  std::string string;
  uint32_t hash;
};

struct ObjFunction : Obj {
  ObjFunction() : Obj{OBJ_FUNCTION} {}

  int arity{};
  Chunk chunk;
  ObjString* name{};
};

using NativeFn = Value (*)(int arg_count, Value* args);

struct ObjNative : Obj {
  explicit ObjNative(NativeFn function) : Obj{OBJ_NATIVE}, function{function} {}

  NativeFn function;
};

inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
}  // namespace lox::bytecode
