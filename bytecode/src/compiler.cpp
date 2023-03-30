#include "compiler.hpp"

#include <iostream>
#include <limits>

#include "vm.hpp"

namespace lox::bytecode {
std::array<Compiler::ParseRule, TOKEN_COUNT> Compiler::rules{{
    {&Compiler::grouping, &Compiler::call,
     Compiler::PREC_CALL},                    // TOKEN_LEFT_PAREN
    {nullptr, nullptr, Compiler::PREC_NONE},  // TOKEN_RIGHT_PAREN
    {nullptr, nullptr, Compiler::PREC_NONE},  // TOKEN_LEFT_BRACE
    {nullptr, nullptr, Compiler::PREC_NONE},  // TOKEN_RIGHT_BRACE
    {nullptr, nullptr, Compiler::PREC_NONE},  // TOKEN_COMMA
    {nullptr, nullptr, Compiler::PREC_NONE},  // TOKEN_DOT
    {&Compiler::unary, &Compiler::binary, Compiler::PREC_TERM},  // TOKEN_MINUS
    {nullptr, &Compiler::binary, Compiler::PREC_TERM},           // TOKEN_PLUS
    {nullptr, nullptr, Compiler::PREC_NONE},                // TOKEN_SEMICOLON
    {nullptr, &Compiler::binary, Compiler::PREC_FACTOR},    // TOKEN_SLASH
    {nullptr, &Compiler::binary, Compiler::PREC_FACTOR},    // TOKEN_STAR
    {&Compiler::unary, nullptr, Compiler::PREC_NONE},       // TOKEN_BANG
    {nullptr, &Compiler::binary, Compiler::PREC_EQUALITY},  // TOKEN_BANG_EQUAL
    {nullptr, nullptr, Compiler::PREC_NONE},                // TOKEN_EQUAL
    {nullptr, &Compiler::binary, Compiler::PREC_EQUALITY},  // TOKEN_EQUAL_EQUAL
    {nullptr, &Compiler::binary, Compiler::PREC_COMPARISON},  // TOKEN_GREATER
    {nullptr, &Compiler::binary,
     Compiler::PREC_COMPARISON},  // TOKEN_GREATER_EQUAL
    {nullptr, &Compiler::binary, Compiler::PREC_COMPARISON},  // TOKEN_LESS
    {nullptr, &Compiler::binary,
     Compiler::PREC_COMPARISON},                          // TOKEN_LESS_EQUAL
    {&Compiler::variable, nullptr, Compiler::PREC_NONE},  // TOKEN_IDENTIFIER
    {&Compiler::string, nullptr, Compiler::PREC_NONE},    // TOKEN_STRING
    {&Compiler::number, nullptr, Compiler::PREC_NONE},    // TOKEN_NUMBER
    {nullptr, &Compiler::and_, Compiler::PREC_AND},       // TOKEN_AND
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_CLASS
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_ELSE
    {&Compiler::literal, nullptr, Compiler::PREC_NONE},   // TOKEN_FALSE
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_FOR
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_FUN
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_IF
    {&Compiler::literal, nullptr, Compiler::PREC_NONE},   // TOKEN_NIL
    {nullptr, &Compiler::or_, Compiler::PREC_OR},         // TOKEN_OR
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_PRINT
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_RETURN
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_SUPER
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_THIS
    {&Compiler::literal, nullptr, Compiler::PREC_NONE},   // TOKEN_TRUE
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_VAR
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_WHILE
    {nullptr, nullptr, Compiler::PREC_NONE},              // TOKEN_ERROR
    {nullptr, nullptr, Compiler::PREC_NONE}               // TOKEN_EOF
}};

Compiler::Compiler(Scanner* scanner, FunctionType type, Compiler* enclosing)
    : scanner_{scanner},
      type_{type},
      enclosing_{enclosing},
      function_{AS_FUNCTION(VM::allocate_object<ObjFunction>())} {
  if (type != TYPE_SCRIPT) {
    function_->name =
        AS_STRING(VM::allocate_object<ObjString>(previous.lexeme_));
  }

  local_count_++;
}

ObjFunction* Compiler::compile() {
  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_EOF, "Expect end of expression.");
  return end_compiler();
}

ObjFunction* Compiler::end_compiler() {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!had_error) {
    current_chunk()->disassemble(
        function_->name != nullptr ? function_->name->string : "<script>");
  }
#endif

  return had_error ? nullptr : function_;
}

void Compiler::emit_byte(uint8_t byte) {
  current_chunk()->write(byte, previous.line_);
}

void Compiler::emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

void Compiler::emit_constant(Value value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

void Compiler::emit_return() {
  emit_byte(OP_NIL);
  emit_byte(OP_RETURN);
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
  if (match(TOKEN_FUN)) {
    fun_declaration();
  } else if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    statement();
  }

  if (panic_mode) {
    synchronize();
  }
}

void Compiler::fun_declaration() {
  const uint8_t global = parse_variable("Expect function name.");
  mark_initialized();
  function(TYPE_FUNCTION);
  define_variable(global);
}

void Compiler::function(FunctionType type) {
  Compiler compiler{scanner_, type, this};
  compiler.begin_scope();

  compiler.consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!compiler.check(TOKEN_RIGHT_PAREN)) {
    do {
      if (++compiler.function_->arity > 255) {
        compiler.error_at_current("Can't have more than 255 parameters.");
      }
      const uint8_t constant =
          compiler.parse_variable("Expect parameter name.");
      compiler.define_variable(constant);
    } while (compiler.match(TOKEN_COMMA));
  }
  compiler.consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  compiler.consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  compiler.block_statement();

  ObjFunction* function = compiler.end_compiler();
  emit_bytes(OP_CONSTANT, make_constant(Value{function}));
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
  } else if (match(TOKEN_RETURN)) {
    return_statement();
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

void Compiler::return_statement() {
  if (type_ == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emit_return();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emit_byte(OP_RETURN);
  }
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

void Compiler::call(bool /*can_assign*/) {
  const uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

uint8_t Compiler::argument_list() {
  uint8_t arg_count = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (arg_count == 255) {
        error("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return arg_count;
}

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
  const TokenType op = previous.type_;
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
  const TokenType op = previous.type_;
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
  switch (previous.type_) {
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
  previous.lexeme_.remove_prefix(1);
  previous.lexeme_.remove_suffix(1);
  emit_constant(VM::allocate_object<ObjString>(previous.lexeme_));
}

void Compiler::variable(bool can_assign) {
  named_variable(previous, can_assign);
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
  const double value = strtod(previous.lexeme_.data(), nullptr);
  emit_constant(Value{value});
}

uint8_t Compiler::parse_variable(std::string_view error_message) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable();
  if (scope_depth_ > 0) {
    return 0;
  }

  return identifier_constant(previous);
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

    if (previous.lexeme_ == local.name.lexeme_) {
      error("Already a variable with this name in this scope.");
    }
  }

  add_local(previous);
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

void Compiler::mark_initialized() {
  if (scope_depth_ == 0) {
    return;
  }

  locals_[local_count_ - 1].depth = scope_depth_;
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
  const ParseFn prefix_rule = get_rule(previous.type_)->prefix;
  if (prefix_rule == nullptr) {
    error("Expect expression.");
    return;
  }

  const bool can_assign = precedence <= PREC_ASSIGNMENT;
  (this->*prefix_rule)(can_assign);

  while (precedence <= get_rule(current.type_)->precedence) {
    advance();
    const ParseFn infix_rule = get_rule(previous.type_)->infix;
    (this->*infix_rule)(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

void Compiler::advance() {
  previous = current;

  for (;;) {
    current = scanner_->scan_token();
    if (current.type_ != TOKEN_ERROR) {
      break;
    }

    error_at_current(current.lexeme_);
  }
}

void Compiler::consume(TokenType type, std::string_view message) {
  if (check(type)) {
    advance();
    return;
  }

  error_at_current(message);
}

bool Compiler::match(TokenType type) {
  if (!check(type)) {
    return false;
  }

  advance();
  return true;
}

void Compiler::synchronize() {
  panic_mode = false;

  while (current.type_ != TOKEN_EOF) {
    if (previous.type_ == TOKEN_SEMICOLON) {
      return;
    }
    switch (current.type_) {
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

void Compiler::error(std::string_view message) { error_at(previous, message); }

void Compiler::error_at_current(std::string_view message) {
  error_at(current, message);
}

void Compiler::error_at(const Token& token, std::string_view message) {
  if (panic_mode) {
    return;
  }
  panic_mode = true;
  std::cerr << "[line " << token.line_ << "] Error";

  if (token.type_ == TOKEN_EOF) {
    std::cerr << " at end";
  } else if (token.type_ == TOKEN_ERROR) {
    // Nothing.
  } else {
    std::cerr << " at '" << token.lexeme_ << "'";
  }

  std::cerr << ": " << message << '\n';
  had_error = true;
}
}  // namespace lox::bytecode
