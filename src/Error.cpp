#include "Error.hpp"

LoxError::LoxError(size_t line, const std::string& message)
    : std::runtime_error("[line " + std::to_string(line) +
                         "] Error: " + message) {}
