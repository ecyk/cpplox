#pragma once

#include "object.hpp"

namespace lox::bytecode {
struct Entry {
  ObjString* key{};
  Value value;
};

class Table {
  static constexpr size_t INITIAL_CAPACITY = 8;
  static constexpr float MAX_LOAD = 0.75F;

  using Entries = std::unique_ptr<Entry[]>;

 public:
  Table() : entries_{std::make_unique<Entry[]>(INITIAL_CAPACITY)} {}

  bool set(ObjString& key, Value value);
  bool get(const ObjString& key, Value* value) const;
  bool del(const ObjString& key);

  void add_all(Table& to) const;
  [[nodiscard]] ObjString* find_string(std::string_view string) const;

 private:
  static Entry* find_entry(const Entries& entries, int capacity,
                           const ObjString* key);
  void adjust_capacity(int capacity);

  int size_{}, capacity_{INITIAL_CAPACITY};
  Entries entries_;
};
}  // namespace lox::bytecode
