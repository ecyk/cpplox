#pragma once

#include <string>

#include "RuntimeError.hpp"

namespace lox::treewalk {
class Token;

inline bool s_had_error{false};
inline bool s_had_runtime_error{false};

void run_file(const std::string& path);
void run_prompt();
void runtime_error(const RuntimeError& error);
void error(const Token& token, const std::string& message);
void error(size_t line, const std::string& message);
void report(size_t line, const std::string& where, const std::string& message);
}  // namespace lox::treewalk
