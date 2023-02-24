#pragma once

#include "value.hpp"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) (is_obj_type(value, OBJ_STRING))

#define AS_STRING(value) (static_cast<ObjString*>(AS_OBJ(value)))

namespace lox::bytecode {
enum ObjType { OBJ_STRING };

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
  template <typename... Args>
  explicit ObjString(Args&&... args)
      : Obj{OBJ_STRING}, string{std::forward<Args>(args)...} {}

  std::string string;
  uint32_t hash{::hash(string)};
};

inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
}  // namespace lox::bytecode
