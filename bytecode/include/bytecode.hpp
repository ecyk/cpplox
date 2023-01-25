#pragma once

#include <string>

namespace lox::bytecode {
int run_file(const std::string& path);
void run_prompt();
}  // namespace lox::bytecode
