#include "compiler.hpp"

#include <iostream>
#include <limits>

#include "object.hpp"
#include "vm.hpp"

namespace lox::bytecode {
Compiler::Compiler(const std::string& source) : scanner_{source} {
  rules_[TOKEN_LEFT_PAREN] = {&Compiler::grouping, nullptr, PREC_NONE};
  rules_[TOKEN_RIGHT_PAREN] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_LEFT_BRACE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_RIGHT_BRACE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_COMMA] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_DOT] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_MINUS] = {&Compiler::unary, &Compiler::binary, PREC_TERM};
  rules_[TOKEN_PLUS] = {nullptr, &Compiler::binary, PREC_TERM};
  rules_[TOKEN_SEMICOLON] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_SLASH] = {nullptr, &Compiler::binary, PREC_FACTOR};
  rules_[TOKEN_STAR] = {nullptr, &Compiler::binary, PREC_FACTOR};
  rules_[TOKEN_BANG] = {&Compiler::unary, nullptr, PREC_NONE};
  rules_[TOKEN_BANG_EQUAL] = {nullptr, &Compiler::binary, PREC_EQUALITY};
  rules_[TOKEN_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_EQUAL_EQUAL] = {nullptr, &Compiler::binary, PREC_EQUALITY};
  rules_[TOKEN_GREATER] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules_[TOKEN_GREATER_EQUAL] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules_[TOKEN_LESS] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules_[TOKEN_LESS_EQUAL] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules_[TOKEN_IDENTIFIER] = {&Compiler::variable, nullptr, PREC_NONE};
  rules_[TOKEN_STRING] = {&Compiler::string, nullptr, PREC_NONE};
  rules_[TOKEN_NUMBER] = {&Compiler::number, nullptr, PREC_NONE};
  rules_[TOKEN_AND] = {nullptr, &Compiler::and_, PREC_AND};
  rules_[TOKEN_CLASS] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_ELSE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_FALSE] = {&Compiler::literal, nullptr, PREC_NONE};
  rules_[TOKEN_FOR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_FUN] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_IF] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_NIL] = {&Compiler::literal, nullptr, PREC_NONE};
  rules_[TOKEN_OR] = {nullptr, &Compiler::or_, PREC_OR};
  rules_[TOKEN_PRINT] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_RETURN] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_SUPER] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_THIS] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_TRUE] = {&Compiler::literal, nullptr, PREC_NONE};
  rules_[TOKEN_VAR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_WHILE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_ERROR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_EOF] = {nullptr, nullptr, PREC_NONE};
}

bool Compiler::compile(Chunk& chunk) {
  chunk_ = &chunk;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_EOF, "Expect end of expression.");

  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!had_error_) {
    current_chunk()->disassemble("code");
  }
#endif

  return !had_error_;
}

void Compiler::emit_byte(uint8_t byte) {
  current_chunk()->write(byte, previous_.line_);
}

void Compiler::emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

void Compiler::emit_constant(Value value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

int Compiler::emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_chunk()->count() - 2;
}

void Compiler::patch_jump(int offset) {
  const unsigned int jump = current_chunk()->count() - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  current_chunk()->set_code(offset, (jump >> 8U) & 0xFFU);
  current_chunk()->set_code(offset + 1, jump & 0xFFU);
}

void Compiler::emit_loop(int loop_start) {
  emit_byte(OP_LOOP);

  const unsigned int offset = current_chunk()->count() - loop_start + 2;
  if (offset > UINT16_MAX) {
    error("Loop body too large.");
  }

  emit_byte((offset >> 8U) & 0xFFU);
  emit_byte(offset & 0xFFU);
}

void Compiler::declaration() {
  if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    statement();
  }

  if (panic_mode_) {
    synchronize();
  }
}

void Compiler::var_declaration() {
  const uint8_t global = parse_variable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  define_variable(global);
}

void Compiler::statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_FOR)) {
    for_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block_statement();
    end_scope();
  } else {
    expression_statement();
  }
}

void Compiler::print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_PRINT);
}

void Compiler::if_statement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  const int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();

  const int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);

  if (match(TOKEN_ELSE)) {
    statement();
  }

  patch_jump(else_jump);
}

void Compiler::while_statement() {
  const int loop_start = current_chunk()->count();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  const int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_loop(loop_start);

  patch_jump(exit_jump);
  emit_byte(OP_POP);
}

void Compiler::for_statement() {
  begin_scope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    expression_statement();
  }

  int loop_start = current_chunk()->count();
  int exit_jump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    const int body_jump = emit_jump(OP_JUMP);
    const int increment_start = current_chunk()->count();
    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement();
  emit_loop(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  end_scope();
}

void Compiler::block_statement() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

void Compiler::expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emit_byte(OP_POP);
}

void Compiler::expression() { parse_precedence(PREC_ASSIGNMENT); }

void Compiler::and_(bool /*can_assign*/) {
  const int end_jump = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP);
  parse_precedence(PREC_AND);

  patch_jump(end_jump);
}

void Compiler::or_(bool /*can_assign*/) {
  const int elseJump = emit_jump(OP_JUMP_IF_FALSE);
  const int endJump = emit_jump(OP_JUMP);

  patch_jump(elseJump);
  emit_byte(OP_POP);

  parse_precedence(PREC_OR);
  patch_jump(endJump);
}

void Compiler::binary(bool /*can_assign*/) {
  const TokenType op = previous_.type_;
  parse_precedence(static_cast<Precedence>(get_rule(op)->precedence + 1));

  switch (op) {
    case TOKEN_BANG_EQUAL:
      emit_bytes(OP_EQUAL, OP_NOT);
      break;
    case TOKEN_EQUAL_EQUAL:
      emit_byte(OP_EQUAL);
      break;
    case TOKEN_GREATER:
      emit_byte(OP_GREATER);
      break;
    case TOKEN_GREATER_EQUAL:
      emit_bytes(OP_LESS, OP_NOT);
      break;
    case TOKEN_LESS:
      emit_byte(OP_LESS);
      break;
    case TOKEN_LESS_EQUAL:
      emit_bytes(OP_GREATER, OP_NOT);
      break;
    case TOKEN_PLUS:
      emit_byte(OP_ADD);
      break;
    case TOKEN_MINUS:
      emit_byte(OP_SUBTRACT);
      break;
    case TOKEN_STAR:
      emit_byte(OP_MULTIPLY);
      break;
    case TOKEN_SLASH:
      emit_byte(OP_DIVIDE);
      break;
    default:
      return;
  }
}

void Compiler::unary(bool /*can_assign*/) {
  const TokenType op = previous_.type_;
  parse_precedence(PREC_UNARY);

  switch (op) {
    case TOKEN_BANG:
      emit_byte(OP_NOT);
      break;
    case TOKEN_MINUS:
      emit_byte(OP_NEGATE);
      break;
    default:
      return;
  }
}

void Compiler::grouping(bool /*can_assign*/) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::literal(bool /*can_assign*/) {
  switch (previous_.type_) {
    case TOKEN_FALSE:
      emit_byte(OP_FALSE);
      break;
    case TOKEN_NIL:
      emit_byte(OP_NIL);
      break;
    case TOKEN_TRUE:
      emit_byte(OP_TRUE);
      break;
    default:
      return;
  }
}

void Compiler::string(bool /*can_assign*/) {
  previous_.lexeme_.remove_prefix(1);
  previous_.lexeme_.remove_suffix(1);
  emit_constant(VM::allocate_object<ObjString>(previous_.lexeme_));
}

void Compiler::variable(bool can_assign) {
  named_variable(previous_, can_assign);
}

void Compiler::named_variable(const Token& name, bool can_assign) {
  uint8_t get_op = OP_GET_GLOBAL;
  uint8_t set_op = OP_SET_GLOBAL;

  int arg = resolve_local(name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else {
    arg = identifier_constant(name);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(set_op, arg);
  } else {
    emit_bytes(get_op, arg);
  }
}

void Compiler::number(bool /*can_assign*/) {
  const double value = strtod(previous_.lexeme_.data(), nullptr);
  emit_constant(Value{value});
}

uint8_t Compiler::parse_variable(std::string_view error_message) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable();
  if (scope_depth_ > 0) {
    return 0;
  }

  return identifier_constant(previous_);
}

void Compiler::declare_variable() {
  if (scope_depth_ == 0) {
    return;
  }

  for (int i = local_count_ - 1; i >= 0; i--) {
    const Local& local = locals_[i];
    if (local.depth != -1 && local.depth < scope_depth_) {
      break;
    }

    if (previous_.lexeme_ == local.name.lexeme_) {
      error("Already a variable with this name in this scope.");
    }
  }

  add_local(previous_);
}

void Compiler::define_variable(uint8_t global) {
  if (scope_depth_ > 0) {
    mark_initialized();
    return;
  }

  emit_bytes(OP_DEFINE_GLOBAL, global);
}

int Compiler::resolve_local(const Token& name) {
  for (int i = local_count_ - 1; i >= 0; i--) {
    const Local& local = locals_[i];
    if (name.lexeme_ == local.name.lexeme_) {
      if (local.depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

uint8_t Compiler::make_constant(Value value) {
  const int index = current_chunk()->add_constant(value);
  if (index > std::numeric_limits<uint8_t>::max()) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return static_cast<uint8_t>(index);
}

uint8_t Compiler::identifier_constant(const Token& name) {
  return make_constant(VM::allocate_object<ObjString>(name.lexeme_));
}

void Compiler::add_local(const Token& name) {
  if (local_count_ == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local& local = locals_[local_count_++];
  local.name = name;
  local.depth = -1;
}

void Compiler::end_scope() {
  scope_depth_--;
  while (local_count_ > 0 && locals_[local_count_ - 1].depth > scope_depth_) {
    emit_byte(OP_POP);
    local_count_--;
  }
}

void Compiler::parse_precedence(Precedence precedence) {
  advance();
  const ParseFn prefix_rule = get_rule(previous_.type_)->prefix;
  if (prefix_rule == nullptr) {
    error("Expect expression.");
    return;
  }

  const bool can_assign = precedence <= PREC_ASSIGNMENT;
  (this->*prefix_rule)(can_assign);

  while (precedence <= get_rule(current_.type_)->precedence) {
    advance();
    const ParseFn infix_rule = get_rule(previous_.type_)->infix;
    (this->*infix_rule)(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

void Compiler::advance() {
  previous_ = current_;

  for (;;) {
    current_ = scanner_.scan_token();
    if (current_.type_ != TOKEN_ERROR) {
      break;
    }

    error_at_current(current_.lexeme_);
  }
}

void Compiler::consume(TokenType type, std::string_view message) {
  if (check(type)) {
    advance();
    return;
  }

  error_at_current(message);
}

bool Compiler::check(TokenType type) const { return current_.type_ == type; }

bool Compiler::match(TokenType type) {
  if (!check(type)) {
    return false;
  }

  advance();
  return true;
}

void Compiler::synchronize() {
  panic_mode_ = false;

  while (current_.type_ != TOKEN_EOF) {
    if (previous_.type_ == TOKEN_SEMICOLON) {
      return;
    }
    switch (current_.type_) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:;  // Do nothing.
    }

    advance();
  }
}

void Compiler::error(std::string_view message) { error_at(previous_, message); }

void Compiler::error_at_current(std::string_view message) {
  error_at(current_, message);
}

void Compiler::error_at(const Token& token, std::string_view message) {
  if (panic_mode_) {
    return;
  }
  panic_mode_ = true;
  std::cerr << "[line " << token.line_ << "] Error";

  if (token.type_ == TOKEN_EOF) {
    std::cerr << " at end";
  } else if (token.type_ == TOKEN_ERROR) {
    // Nothing.
  } else {
    std::cerr << " at '" << token.lexeme_ << "'";
  }

  std::cerr << ": " << message << '\n';
  had_error_ = true;
}
}  // namespace lox::bytecode
