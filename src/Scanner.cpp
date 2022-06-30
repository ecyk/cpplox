#include "Scanner.hpp"

#include "Lox.hpp"

namespace lox::treewalk {
static const std::unordered_map<std::string, TokenType> keywords_ = {  // NOLINT
    {"and", TokenType::AND},       {"class", TokenType::CLASS},
    {"else", TokenType::ELSE},     {"false", TokenType::FALSE},
    {"for", TokenType::FOR},       {"fun", TokenType::FUN},
    {"if", TokenType::IF},         {"nil", TokenType::NIL},
    {"or", TokenType::OR},         {"print", TokenType::PRINT},
    {"return", TokenType::RETURN}, {"super", TokenType::SUPER},
    {"this", TokenType::THIS},     {"true", TokenType::TRUE},
    {"var", TokenType::VAR},       {"while", TokenType::WHILE}};

Scanner::Scanner(std::string source) : source_{std::move(source)} {}

std::vector<Token> Scanner::scan_tokens() {
  while (!is_at_end()) {
    start_ = current_;
    scan_token();
  }

  tokens_.emplace_back(TokenType::EOF_, "", line_);
  return tokens_;
}

void Scanner::scan_token() {
  char c = advance();  // NOLINT
  switch (c) {
    case '(':
      add_token(TokenType::LEFT_PAREN);
      break;
    case ')':
      add_token(TokenType::RIGHT_PAREN);
      break;
    case '{':
      add_token(TokenType::LEFT_BRACE);
      break;
    case '}':
      add_token(TokenType::RIGHT_BRACE);
      break;
    case ',':
      add_token(TokenType::COMMA);
      break;
    case '.':
      add_token(TokenType::DOT);
      break;
    case '-':
      add_token(TokenType::MINUS);
      break;
    case '+':
      add_token(TokenType::PLUS);
      break;
    case ';':
      add_token(TokenType::SEMICOLON);
      break;
    case '*':
      add_token(TokenType::STAR);
      break;
    case '!':
      add_token(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
      break;
    case '=':
      add_token(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
      break;
    case '<':
      add_token(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
      break;
    case '>':
      add_token(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
      break;
    case '/':
      if (match('/')) {
        comment();
      } else if (match('*')) {
        multiline_comment();
      } else {
        add_token(TokenType::SLASH);
      }
      break;
    case ' ':
    case '\r':
    case '\t':
      // Ignore whitespace
      break;
    case '\n':
      ++line_;
      break;
    case '"':
      string();
      break;
    default:
      if (std::isdigit(c) != 0) {
        number();
      } else if ((std::isalpha(c) != 0) || c == '_') {
        identifier();
      } else {
        error(line_, "Unexpected character.");
      }
      break;
  }
}

void Scanner::add_token(TokenType type) {
  tokens_.emplace_back(type, source_.substr(start_, current_ - start_), line_);
}

void Scanner::comment() {
  // A comment goes until the end of the line
  while (peek() != '\n' && !is_at_end()) {
    advance();
  }
}

void Scanner::multiline_comment() {
  size_t start_line = line_;

  // A multiline comment goes until the next "*/"
  while ((peek() != '*' || peek_next() != '/') && !is_at_end()) {
    if (peek() == '\n') {
      ++line_;
    }
    advance();
  }

  if (is_at_end()) {
    error(start_line, "Unterminated multiline comment.");
    return;
  }

  // Consume the "*"
  advance();

  // Consume the "/"
  advance();
}

void Scanner::string() {
  size_t start_line = line_;

  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') {
      ++line_;
    }
    advance();
  }

  if (is_at_end()) {
    error(start_line, "Unterminated string.");
    return;
  }

  // The closing "
  advance();

  add_token(TokenType::STRING);
}

void Scanner::number() {
  while (std::isdigit(peek()) != 0) {
    advance();
  }

  // Look for a fractional part
  if (peek() == '.' && (std::isdigit(peek_next()) != 0)) {
    // Consume the "."
    advance();

    while (std::isdigit(peek()) != 0) {
      advance();
    }
  }

  add_token(TokenType::NUMBER);
}

void Scanner::identifier() {
  while ((std::isalnum(peek()) != 0) || peek() == '_') {
    advance();
  }

  if (auto it =  // NOLINT
      keywords_.find(source_.substr(start_, current_ - start_));
      it != keywords_.end()) {
    add_token(it->second);
    return;
  }

  add_token(TokenType::IDENTIFIER);
}

char Scanner::peek() const {
  if (is_at_end()) {
    return '\0';
  }
  return source_.at(current_);
}

char Scanner::peek_next() const {
  if (current_ + 1 >= source_.size()) {
    return '\0';
  }
  return source_.at(current_ + 1);
}

bool Scanner::match(char expected) {
  if (is_at_end()) {
    return false;
  }
  if (source_.at(current_) != expected) {
    return false;
  }

  ++current_;
  return true;
}

char Scanner::advance() { return source_.at(current_++); }

bool Scanner::is_at_end() const { return current_ >= source_.size(); }
}  // namespace lox::treewalk
