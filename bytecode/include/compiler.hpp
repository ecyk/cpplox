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

  using ParseFn = void (Compiler::*)(bool can_assign);

  struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
  };

 public:
  explicit Compiler(const std::string& source);

  bool compile(Chunk& chunk);

 private:
  void emit_byte(uint8_t byte);
  void emit_bytes(uint8_t byte1, uint8_t byte2);
  void emit_constant(Value value);
  void emit_return() { emit_byte(OP_RETURN); }

  Chunk* current_chunk() { return chunk_; }

  void declaration();
  void var_declaration();
  void statement();
  void print_statement();
  void expression_statement();
  void expression();
  void binary(bool can_assign);
  void unary(bool can_assign);
  void grouping(bool can_assign);
  void literal(bool can_assign);
  void string(bool can_assign);
  void variable(bool can_assign);
  void named_variable(const Token& name, bool can_assign);
  void number(bool can_assign);

  uint8_t parse_variable(std::string_view error_message);
  void define_variable(uint8_t global);

  uint8_t make_constant(Value value);
  uint8_t identifier_constant(const Token& name);

  ParseRule* get_rule(TokenType type) { return &rules_.at(type); }
  void parse_precedence(Precedence precedence);

  void advance();
  void consume(TokenType type, std::string_view message);
  [[nodiscard]] bool check(TokenType type) const;
  bool match(TokenType type);

  void synchronize();

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
