#include "compiler.hpp"

#include <iomanip>
#include <iostream>

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
  rules_[TOKEN_BANG] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_BANG_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_EQUAL_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_GREATER] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_GREATER_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_LESS] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_LESS_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_IDENTIFIER] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_STRING] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_NUMBER] = {&Compiler::number, nullptr, PREC_NONE};
  rules_[TOKEN_AND] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_CLASS] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_ELSE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_FALSE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_FOR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_FUN] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_IF] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_NIL] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_OR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_PRINT] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_RETURN] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_SUPER] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_THIS] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_TRUE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_VAR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_WHILE] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_ERROR] = {nullptr, nullptr, PREC_NONE};
  rules_[TOKEN_EOF] = {nullptr, nullptr, PREC_NONE};
}

bool Compiler::compile(Chunk& chunk) {
  chunk_ = &chunk;

  advance();
  expression();
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
  int index = current_chunk()->add_constant(value);
  if (index > std::numeric_limits<uint8_t>::max()) {
    error("Too many constants in one chunk.");
    index = 0;
  }

  emit_bytes(OP_CONSTANT, index);
}

void Compiler::expression() { parse_precedence(PREC_ASSIGNMENT); }

void Compiler::binary() {
  const TokenType op = previous_.type_;
  parse_precedence(static_cast<Precedence>(get_rule(op)->precedence + 1));

  switch (op) {
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

void Compiler::unary() {
  const TokenType op = previous_.type_;
  parse_precedence(PREC_UNARY);

  switch (op) {
    case TOKEN_MINUS:
      emit_byte(OP_NEGATE);
      break;
    default:
      return;
  }
}

void Compiler::grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::number() {
  const double value = strtod(previous_.lexeme_.data(), nullptr);
  emit_constant(value);
}

void Compiler::parse_precedence(Precedence precedence) {
  advance();
  const ParseFn prefix_rule = get_rule(previous_.type_)->prefix;
  if (prefix_rule == nullptr) {
    error("Expect expression.");
    return;
  }

  (this->*prefix_rule)();

  while (precedence <= get_rule(current_.type_)->precedence) {
    advance();
    const ParseFn infix_rule = get_rule(previous_.type_)->infix;
    (this->*infix_rule)();
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
  if (current_.type_ == type) {
    advance();
    return;
  }

  error_at_current(message);
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
