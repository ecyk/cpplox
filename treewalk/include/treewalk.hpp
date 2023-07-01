#pragma once

#include <string>

#include "runtime_error.hpp"

namespace lox::treewalk {
int run_file(const std::string& path);
void run_prompt();
void runtime_error(const RuntimeError& error);
void error(const lox::Token& token, const std::string& message);
void error(int line, const std::string& message);
void report(int line, const std::string& where, const std::string& message);
}  // namespace lox::treewalk
