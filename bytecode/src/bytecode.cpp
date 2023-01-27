#include "bytecode.hpp"

#include <fstream>
#include <iostream>

#include "compiler.hpp"
#include "vm.hpp"

namespace lox::bytecode {
static InterpretResult run(const std::string& source) {
  Chunk chunk;

  Compiler compiler{source};
  if (!compiler.compile(chunk)) {
    return INTERPRET_COMPILE_ERROR;
  }

  VM vm;
  return vm.interpret(chunk);
}

int run_file(const std::string& path) {
  std::ifstream file_stream{path};
  file_stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  const std::string source{std::istreambuf_iterator<char>{file_stream},
                           std::istreambuf_iterator<char>{}};

  const InterpretResult result = run(source);

  if (result == INTERPRET_COMPILE_ERROR) {
    return 65;
  }
  if (result == INTERPRET_RUNTIME_ERROR) {
    return 70;
  }

  return 0;
}

void run_prompt() {
  std::string source_line;
  for (;;) {
    std::cout << "> ";

    std::getline(std::cin, source_line);
    if (source_line.empty()) {
      break;
    }

    run(source_line);
  }
}
}  // namespace lox::bytecode
