#pragma once

#include <array>
#include <limits>
#include <optional>

#include "object.hpp"
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
    bool is_captured{};
  };

  struct Upvalue {
    uint8_t index{};
    bool is_local{};
  };

  enum FunctionType {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
  };

  static std::array<ParseRule, TOKEN_COUNT> rules;

  inline static Token previous, current;
  inline static bool had_error{};
  inline static bool panic_mode{};

  struct ClassCompiler {
    ClassCompiler* enclosing{};
    bool has_super_class{};
  };

  inline static ClassCompiler* current_class_compiler{};

 public:
  explicit Compiler(Scanner& scanner, FunctionType type = TYPE_SCRIPT,
                    Compiler* enclosing = nullptr);

  ObjFunction* compile();

  void mark_compiler_roots();

 private:
  ObjFunction* end_compiler();

  void emit_byte(uint8_t byte);
  void emit_bytes(uint8_t byte1, uint8_t byte2);
  void emit_constant(Value value);
  void emit_return();
  uint32_t emit_jump(uint8_t instruction);
  void patch_jump(uint32_t offset);
  void emit_loop(uint32_t loop_start);

  void declaration();
  void class_declaration();
  void method();
  void fun_declaration();
  void function(FunctionType type);
  void var_declaration();
  void statement();
  void print_statement();
  void if_statement();
  void return_statement();
  void while_statement();
  void for_statement();
  void block_statement();
  void expression_statement();
  void expression();
  void call(bool can_assign);
  uint8_t argument_list();
  void super_(bool can_assign);
  void this_(bool can_assign);
  void dot(bool can_assign);
  void and_(bool can_assign);
  void or_(bool can_assign);
  void binary(bool can_assign);
  void unary(bool can_assign);
  void grouping(bool can_assign);
  void literal(bool can_assign);
  void string(bool can_assign);
  void variable(bool can_assign);
  void named_variable(const lox::Token& name, bool can_assign);
  void number(bool can_assign);

  uint8_t parse_variable(std::string_view error_message);
  void declare_variable();
  void define_variable(uint8_t global);
  std::optional<uint8_t> resolve_local(const lox::Token& name);
  std::optional<uint8_t> resolve_upvalue(const lox::Token& name);
  void add_local(const lox::Token& name);
  std::optional<uint8_t> add_upvalue(uint8_t index, bool is_local);
  void mark_initialized();
  uint8_t make_constant(Value value);
  uint8_t identifier_constant(const lox::Token& name);
  void begin_scope() { scope_depth_++; }
  void end_scope();

  Chunk* current_chunk() { return &function_->chunk; }
  uint32_t current_chunk_size() {
    return static_cast<uint32_t>(current_chunk()->get_codes().size());
  }

  static ParseRule* get_rule(TokenType type) { return &rules[type]; }
  void parse_precedence(Precedence precedence);

  void advance();
  void consume(TokenType type, std::string_view message);
  [[nodiscard]] static bool check(TokenType type) {
    return current.type == type;
  }
  bool match(TokenType type);

  void synchronize();

  static void error(std::string_view message);
  static void error_at_current(std::string_view message);
  static void error_at(const lox::Token& token, std::string_view message);

  ObjFunction* function_{};
  FunctionType type_;

  std::array<Local, UINT8_COUNT> locals_;
  uint16_t local_count_{};
  std::array<Upvalue, UINT8_COUNT> upvalues_;
  int scope_depth_{};

  Compiler* enclosing_{};
  Scanner* scanner_;
};

inline Compiler* g_current_compiler{};
}  // namespace lox::bytecode
