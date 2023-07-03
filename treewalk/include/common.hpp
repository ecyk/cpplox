#pragma once

#include <memory>
#include <string>
#include <vector>

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

namespace lox::treewalk {
struct Hash {
  using is_transparent [[maybe_unused]] = void;

  [[nodiscard]] size_t operator()(const char* string) const {
    return std::hash<std::string_view>{}(string);
  }
  [[nodiscard]] size_t operator()(std::string_view string) const {
    return std::hash<std::string_view>{}(string);
  }
  [[nodiscard]] size_t operator()(const std::string& string) const {
    return std::hash<std::string>{}(string);
  }
};
}  // namespace lox::treewalk
