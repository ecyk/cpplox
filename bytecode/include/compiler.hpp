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

  struct Local {
    Token name;
    int depth{};
  };

  static constexpr int UINT8_COUNT = 256;

 public:
  explicit Compiler(const std::string& source);

  bool compile(Chunk& chunk);

 private:
  void emit_byte(uint8_t byte);
  void emit_bytes(uint8_t byte1, uint8_t byte2);
  void emit_constant(Value value);
  void emit_return() { emit_byte(OP_RETURN); }
  int emit_jump(uint8_t instruction);
  void patch_jump(int offset);
  void emit_loop(int loop_start);

  void declaration();
  void var_declaration();
  void statement();
  void print_statement();
  void if_statement();
  void while_statement();
  void for_statement();
  void block_statement();
  void expression_statement();
  void expression();
  void and_(bool can_assign);
  void or_(bool can_assign);
  void binary(bool can_assign);
  void unary(bool can_assign);
  void grouping(bool can_assign);
  void literal(bool can_assign);
  void string(bool can_assign);
  void variable(bool can_assign);
  void named_variable(const Token& name, bool can_assign);
  void number(bool can_assign);

  uint8_t parse_variable(std::string_view error_message);
  void declare_variable();
  void define_variable(uint8_t global);
  int resolve_local(const Token& name);

  uint8_t make_constant(Value value);
  uint8_t identifier_constant(const Token& name);
  void add_local(const Token& name);
  void mark_initialized() { locals_[local_count_ - 1].depth = scope_depth_; }

  void begin_scope() { scope_depth_++; }
  void end_scope();

  ParseRule* get_rule(TokenType type) { return &rules_[type]; }
  void parse_precedence(Precedence precedence);

  void advance();
  void consume(TokenType type, std::string_view message);
  [[nodiscard]] bool check(TokenType type) const;
  bool match(TokenType type);

  void synchronize();

  void error(std::string_view message);
  void error_at_current(std::string_view message);
  void error_at(const Token& token, std::string_view message);

  Chunk* current_chunk() { return chunk_; }

  Scanner scanner_;

  Token previous_, current_;
  bool had_error_{};
  bool panic_mode_{};

  std::array<ParseRule, TOKEN_COUNT> rules_{};

  std::array<Local, UINT8_COUNT> locals_{};
  int local_count_{};
  int scope_depth_{};

  Chunk* chunk_{};
};
}  // namespace lox::bytecode
