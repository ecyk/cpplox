#pragma once

#include "value.hpp"

namespace lox::bytecode {
struct ObjString;

struct Entry {
  ObjString* key{};
  Value value;
};

class Table {
  static constexpr int INITIAL_CAPACITY = 8;
  static constexpr float MAX_LOAD = 0.75F;

  using Entries = std::unique_ptr<Entry[]>;

 public:
  Table() : entries_{std::make_unique<Entry[]>(INITIAL_CAPACITY)} {}

  bool set(ObjString* key, Value value);
  bool get(ObjString* key, Value* value) const;
  bool del(ObjString* key);
  void add_all(Table& to) const;

  [[nodiscard]] ObjString* find_string(std::string_view string,
                                       uint32_t hash) const;
  void remove_white();

  [[nodiscard]] const Entries& get_entries() const { return entries_; }
  [[nodiscard]] int get_capacity() const { return capacity_; }

 private:
  static Entry* find_entry(const Entries& entries, int capacity,
                           const ObjString* key);
  void adjust_capacity(int capacity);

  int size_{}, capacity_{INITIAL_CAPACITY};
  Entries entries_;
};
}  // namespace lox::bytecode
