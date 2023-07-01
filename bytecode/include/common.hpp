#pragma once

#include <memory>
#include <string>
#include <vector>

#define NAN_BOXING
// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

inline static constexpr int UINT8_COUNT = 256;

inline uint32_t hash(std::string_view key) {
  uint32_t hash = 2166136261;
  for (const char i : key) {
    hash ^= static_cast<uint8_t>(i);
    hash *= 16777619;
  }

  return hash;
}
