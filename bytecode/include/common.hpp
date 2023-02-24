#pragma once

#include <memory>
#include <string>
#include <vector>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

inline uint32_t hash(std::string_view key) {
  uint32_t hash = 2166136261;
  for (const char i : key) {
    hash ^= (uint8_t)i;
    hash *= 16777619;
  }

  return hash;
}
