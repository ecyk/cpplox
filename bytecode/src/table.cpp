#include "table.hpp"

#include "object.hpp"

namespace lox::bytecode {
bool Table::set(ObjString* key, Value value) {
  if (size_ + 1 > static_cast<int>(static_cast<float>(capacity_) * MAX_LOAD)) {
    adjust_capacity(capacity_ * 2);
  }

  Entry* entry = find_entry(entries_, capacity_, key);
  const bool is_new_key = entry->key == nullptr;
  if (is_new_key) {
    size_++;
  }

  entry->key = key;
  entry->value = value;
  return is_new_key;
}

bool Table::get(ObjString* key, Value* value) const {
  if (size_ == 0) {
    return false;
  }

  Entry* entry = find_entry(entries_, capacity_, key);
  if (entry->key == nullptr) {
    return false;
  }

  *value = entry->value;
  return true;
}

bool Table::del(ObjString* key) {
  if (size_ == 0) {
    return false;
  }

  Entry* entry = find_entry(entries_, capacity_, key);
  if (entry->key == nullptr) {
    return false;
  }

  entry->key = nullptr;
  entry->value = Value{true};
  return true;
}

void Table::add_all(Table& to) const {
  for (int i = 0; i < capacity_; i++) {
    Entry* entry = &entries_[i];
    if (entry->key != nullptr) {
      to.set(entry->key, entry->value);
    }
  }
}

ObjString* Table::find_string(std::string_view string, uint32_t hash) const {
  if (size_ == 0) {
    return nullptr;
  }

  uint32_t index = hash % capacity_;
  for (;;) {
    Entry* entry = &entries_[index];
    if (entry->key == nullptr) {
      if (IS_NIL(entry->value)) {
        return nullptr;
      }
    } else if (entry->key->hash == hash && entry->key->string == string) {
      return entry->key;
    }

    index = (index + 1) % capacity_;
  }
}

Entry* Table::find_entry(const Entries& entries, int capacity,
                         const ObjString* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = nullptr;

  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == nullptr) {
      if (IS_NIL(entry->value)) {
        return tombstone != nullptr ? tombstone : entry;
      } else {
        if (tombstone == nullptr) {
          tombstone = entry;
        }
      }
    } else if (entry->key == key) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

void Table::adjust_capacity(int capacity) {
  auto entries = std::make_unique<Entry[]>(capacity);

  size_ = 0;
  for (int i = 0; i < capacity_; i++) {
    Entry* entry = &entries_[i];
    if (entry->key == nullptr) {
      continue;
    }

    Entry* dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    size_++;
  }

  capacity_ = capacity;
  entries_ = std::move(entries);
}
}  // namespace lox::bytecode
