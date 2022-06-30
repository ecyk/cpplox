#pragma once

#include <string>

namespace lox::treewalk {
inline bool s_had_error = false;

void run_file(const std::string& path);
void run_prompt();
void error(size_t line, const std::string& message);
void report(size_t line, const std::string& where, const std::string& message);
}  // namespace lox::treewalk
