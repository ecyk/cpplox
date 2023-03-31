#include "scanner.hpp"

#include <cctype>
#include <cstring>

namespace lox::bytecode {
Token Scanner::scan_token() {
  for (;;) {
    switch (const char c = peek()) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        continue;
      case '\n':
        line_++;
        advance();
        continue;
      case '/':
        if (peek_next() == '/') {
          while (peek() != '\n' && !is_at_end()) {
            advance();
          }
        } else if (peek_next() == '*') {
          advance();
          advance();

          while ((peek() != '*' || peek_next() != '/') && !is_at_end()) {
            if (peek() == '\n') {
              ++line_;
            }
            advance();
          }

          if (is_at_end()) {
            return error_token("Unterminated multiline comment.");
          }

          advance();
          advance();
        } else {
          break;
        }
        continue;
      default:
        break;
    }
    break;
  }

  start_ = current_;

  if (is_at_end()) {
    return make_token(TOKEN_EOF);
  }

  const char c = advance();

  if (std::isalpha(c) != 0 || c == '_') {
    return identifier();
  }
  if (std::isdigit(c) != 0) {
    return number();
  }

  switch (c) {
    case '(':
      return make_token(TOKEN_LEFT_PAREN);
    case ')':
      return make_token(TOKEN_RIGHT_PAREN);
    case '{':
      return make_token(TOKEN_LEFT_BRACE);
    case '}':
      return make_token(TOKEN_RIGHT_BRACE);
    case ';':
      return make_token(TOKEN_SEMICOLON);
    case ',':
      return make_token(TOKEN_COMMA);
    case '.':
      return make_token(TOKEN_DOT);
    case '-':
      return make_token(TOKEN_MINUS);
    case '+':
      return make_token(TOKEN_PLUS);
    case '/':
      return make_token(TOKEN_SLASH);
    case '*':
      return make_token(TOKEN_STAR);
    case '!':
      return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"':
      return string();
    default:
      break;
  }

  return error_token("Unexpected character.");
}

Token Scanner::identifier() {
  while ((std::isalpha(peek()) != 0) || (std::isdigit(peek()) != 0) ||
         (peek() == '_')) {
    advance();
  }
  return make_token(identifier_type());
}

TokenType Scanner::identifier_type() const {
  auto check_keyword = [&](int start, int length, const char* rest,
                           TokenType type) {
    if (current_ - start_ == start + length &&
        memcmp(start_ + start, rest, length) == 0) {
      return type;
    }

    return TOKEN_IDENTIFIER;
  };

  switch (*start_) {
    case 'a':
      return check_keyword(1, 2, "nd", TOKEN_AND);
    case 'c':
      return check_keyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
      return check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (current_ - start_ > 1) {
        switch (*(start_ + 1)) {
          case 'a':
            return check_keyword(2, 3, "lse", TOKEN_FALSE);
          case 'o':
            return check_keyword(2, 1, "r", TOKEN_FOR);
          case 'u':
            return check_keyword(2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 'i':
      return check_keyword(1, 1, "f", TOKEN_IF);
    case 'n':
      return check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o':
      return check_keyword(1, 1, "r", TOKEN_OR);
    case 'p':
      return check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
      return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
      return check_keyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (current_ - start_ > 1) {
        switch (*(start_ + 1)) {
          case 'h':
            return check_keyword(2, 2, "is", TOKEN_THIS);
          case 'r':
            return check_keyword(2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'v':
      return check_keyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
      return check_keyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

Token Scanner::number() {
  while (std::isdigit(peek()) != 0) {
    advance();
  }

  if (peek() == '.' && std::isdigit(peek_next()) != 0) {
    advance();

    while (std::isdigit(peek()) != 0) {
      advance();
    }
  }

  return make_token(TOKEN_NUMBER);
}

Token Scanner::string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') {
      line_++;
    }
    advance();
  }

  if (is_at_end()) {
    return error_token("Unterminated string.");
  }

  advance();
  return make_token(TOKEN_STRING);
}

char Scanner::peek_next() const {
  if (is_at_end()) {
    return '\0';
  }
  return *(current_ + 1);
}

bool Scanner::match(char expected) {
  if (is_at_end()) {
    return false;
  }
  if (*current_ != expected) {
    return false;
  }
  current_++;
  return true;
}
}  // namespace lox::bytecode
