#include "bytecode.hpp"

#include <fstream>
#include <iostream>

#include "compiler.hpp"
#include "vm.hpp"

namespace lox::bytecode {
static InterpretResult run(const std::string& source) {
  Scanner scanner{source};
  Compiler compiler{scanner};

  ObjFunction* function = compiler.compile();
  if (function == nullptr) {
    return INTERPRET_COMPILE_ERROR;
  }

  return g_vm.interpret(function);
}

int run_file(const std::string& path) {
  std::ifstream file_stream{path};
  file_stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  const std::string source{std::istreambuf_iterator<char>{file_stream},
                           std::istreambuf_iterator<char>{}};

  const InterpretResult result = run(source);
  g_vm.free_objects();

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
  g_vm.free_objects();
}
}  // namespace lox::bytecode
