#pragma once

#include <array>

#include "chunk.hpp"
#include "scanner.hpp"

namespace lox::bytecode {
class Compiler {
  enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
  };

  using ParseFn = void (Compiler::*)();

  struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
  };

 public:
  Compiler(const std::string& source);

  bool compile(Chunk& chunk);

 private:
  void emit_byte(uint8_t byte);
  void emit_bytes(uint8_t byte1, uint8_t byte2);
  void emit_constant(Value value);
  void emit_return() { emit_byte(OP_RETURN); }

  Chunk* current_chunk() { return chunk_; }

  void expression();
  void binary();
  void unary();
  void grouping();
  void number();

  ParseRule* get_rule(TokenType type) { return &rules_[type]; }
  void parse_precedence(Precedence precedence);

  void advance();
  void consume(TokenType type, std::string_view message);

  void error(std::string_view message);
  void error_at_current(std::string_view message);
  void error_at(const Token& token, std::string_view message);

  Scanner scanner_;

  Token previous_, current_;
  bool had_error_{};
  bool panic_mode_{};

  std::array<ParseRule, TOKEN_COUNT> rules_{};

  Chunk* chunk_{};
};
}  // namespace lox::bytecode
