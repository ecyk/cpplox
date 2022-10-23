#include "Parser.hpp"

#include "Lox.hpp"

namespace lox::treewalk {
Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

std::vector<Scope<Stmt>> Parser::parse() {
  std::vector<Scope<Stmt>> statements;
  while (!is_at_end()) {
    statements.push_back(declaration());
  }

  return statements;
}

Scope<Stmt> Parser::declaration() {
  try {
    if (match(TokenType::CLASS)) {
      return class_declaration();
    }
    if (match(TokenType::FUN)) {
      return fun_declaration();
    }
    if (match(TokenType::VAR)) {
      return var_declaration();
    }

    return statement();
  } catch (const ParseError&) {
    synchronize();
    return {};
  }
}

Scope<Stmt> Parser::class_declaration() {
  Token name = consume(TokenType::IDENTIFIER, "Expect class name.");
  consume(TokenType::LEFT_BRACE, "Expect '{' before class body.");

  std::vector<stmt::Function> methods;
  while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
    methods.push_back(fun_declaration("method"));
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");

  return std::make_unique<stmt::Class>(name, std::move(methods));
}

stmt::Function Parser::fun_declaration(const std::string& kind) {
  Token& name = consume(TokenType::IDENTIFIER, "Expect " + kind + " name.");
  consume(TokenType::LEFT_PAREN, "Expect '(' after " + kind + " name.");
  std::vector<Token> parameters;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      constexpr size_t max_parameter_count = 255;
      if (parameters.size() >= max_parameter_count) {
        error(peek(), "Can't have more than 255 parameters.");
      }

      parameters.push_back(
          consume(TokenType::IDENTIFIER, "Expect parameter name."));
    } while (match(TokenType::COMMA));
  }
  consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");

  consume(TokenType::LEFT_BRACE, "Expect '{' before " + kind + " body.");
  auto body = block();
  return {name, std::move(parameters), std::move(body)};
}

Scope<Stmt> Parser::fun_declaration() {
  return std::make_unique<stmt::Function>(fun_declaration("function"));
}

Scope<Stmt> Parser::var_declaration() {
  Token& name = consume(TokenType::IDENTIFIER, "Expect variable name.");

  Scope<Expr> initializer;
  if (match(TokenType::EQUAL)) {
    initializer = expression();
  }

  consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
  return std::make_unique<stmt::Var>(name, std::move(initializer));
}

Scope<Stmt> Parser::statement() {
  if (match(TokenType::FOR)) {
    return for_statement();
  }
  if (match(TokenType::IF)) {
    return if_statement();
  }
  if (match(TokenType::PRINT)) {
    return print_statement();
  }
  if (match(TokenType::RETURN)) {
    return return_statement();
  }
  if (match(TokenType::WHILE)) {
    return while_statement();
  }
  if (match(TokenType::LEFT_BRACE)) {
    return block_statement();
  }

  return expression_statement();
}

Scope<Stmt> Parser::for_statement() {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
  Scope<Stmt> initializer;
  if (match(TokenType::SEMICOLON)) {
  } else if (match(TokenType::VAR)) {
    initializer = var_declaration();
  } else {
    initializer = expression_statement();
  }

  Scope<Expr> condition;
  if (!check(TokenType::SEMICOLON)) {
    condition = expression();
  }
  consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");

  Scope<Expr> increment;
  if (!check(TokenType::RIGHT_PAREN)) {
    increment = expression();
  }
  consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");
  auto body = statement();

  if (increment) {
    std::vector<Scope<Stmt>> loop_statements;
    loop_statements.push_back(std::move(body));
    loop_statements.push_back(
        std::make_unique<stmt::Expression>(std::move(increment)));
    body = std::make_unique<stmt::Block>(std::move(loop_statements));
  }

  if (!condition) {
    condition = std::make_unique<expr::Literal>(true);
  }
  body = std::make_unique<stmt::While>(std::move(condition), std::move(body));

  if (initializer) {
    std::vector<Scope<Stmt>> block;
    block.push_back(std::move(initializer));
    block.push_back(std::move(body));
    body = std::make_unique<stmt::Block>(std::move(block));
  }

  return body;
}

Scope<Stmt> Parser::if_statement() {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");

  auto then_branch = statement();
  Scope<Stmt> else_branch;
  if (match(TokenType::ELSE)) {
    else_branch = statement();
  }

  return std::make_unique<stmt::If>(
      std::move(condition), std::move(then_branch), std::move(else_branch));
}

Scope<Stmt> Parser::print_statement() {
  auto value = expression();
  consume(TokenType::SEMICOLON, "Expect ';' after value.");
  return std::make_unique<stmt::Print>(std::move(value));
}

Scope<Stmt> Parser::return_statement() {
  Token& keyword = previous();
  Scope<Expr> value;
  if (!check(TokenType::SEMICOLON)) {
    value = expression();
  }

  consume(TokenType::SEMICOLON, "Expect ';' after return value.");
  return std::make_unique<stmt::Return>(keyword, std::move(value));
}

Scope<Stmt> Parser::while_statement() {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
  auto body = statement();

  return std::make_unique<stmt::While>(std::move(condition), std::move(body));
}

Scope<Stmt> Parser::block_statement() {
  return std::make_unique<stmt::Block>(block());
}

Scope<Stmt> Parser::expression_statement() {
  auto expr = expression();
  consume(TokenType::SEMICOLON, "Expect ';' after expression.");
  return std::make_unique<stmt::Expression>(std::move(expr));
}

std::vector<Scope<Stmt>> Parser::block() {
  std::vector<Scope<Stmt>> statements;

  while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
    statements.push_back(declaration());
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
  return statements;
}

Scope<Expr> Parser::finish_call(Scope<Expr> callee) {
  std::vector<Scope<Expr>> arguments;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      constexpr size_t max_argument_count = 255;
      if (arguments.size() >= max_argument_count) {
        error(peek(), "Can't have more than 255 arguments.");
      }
      arguments.push_back(expression());
    } while (match(TokenType::COMMA));
  }

  Token& paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");

  return std::make_unique<expr::Call>(std::move(callee), paren,
                                      std::move(arguments));
}

Scope<Expr> Parser::expression() { return assignment(); }

Scope<Expr> Parser::assignment() {
  auto expr = logic_or();

  if (match(TokenType::EQUAL)) {
    Token& equals = previous();
    auto value = assignment();

    if (auto* expr_ptr = dynamic_cast<expr::Variable*>(expr.get());
        expr_ptr != nullptr) {
      Token& name = expr_ptr->name_;
      return std::make_unique<expr::Assign>(name, std::move(value));
    } else if (auto* expr_ptr = dynamic_cast<expr::Get*>(expr.get());
               expr_ptr != nullptr) {
      Token& name = expr_ptr->name_;
      return std::make_unique<expr::Set>(std::move(expr_ptr->object_), name,
                                         std::move(value));
    }

    error(equals, "Invalid assignment target.");
  }

  return expr;
}

Scope<Expr> Parser::logic_or() {
  auto expr = logic_and();

  while (match(TokenType::OR)) {
    Token& op = previous();
    auto right = logic_and();
    expr =
        std::make_unique<expr::Logical>(std::move(expr), op, std::move(right));
  }

  return expr;
}

Scope<Expr> Parser::logic_and() {
  auto expr = equality();

  while (match(TokenType::AND)) {
    Token& op = previous();
    auto right = equality();
    expr =
        std::make_unique<expr::Logical>(std::move(expr), op, std::move(right));
  }

  return expr;
}

Scope<Expr> Parser::equality() {
  auto expr = comparison();

  while (match(TokenType::BANG_EQUAL) || match(TokenType::EQUAL_EQUAL)) {
    Token& op = previous();
    auto right = comparison();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

Scope<Expr> Parser::comparison() {
  auto expr = term();

  while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
         match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
    Token& op = previous();
    auto right = term();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

Scope<Expr> Parser::term() {
  auto expr = factor();

  while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
    Token& op = previous();
    auto right = factor();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

Scope<Expr> Parser::factor() {
  auto expr = unary();

  while (match(TokenType::SLASH) || match(TokenType::STAR)) {
    Token& op = previous();
    auto right = unary();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

Scope<Expr> Parser::unary() {
  if (match(TokenType::BANG) || match(TokenType::MINUS)) {
    Token& op = previous();
    auto right = unary();
    return std::make_unique<expr::Unary>(op, std::move(right));
  }

  return call();
}

Scope<Expr> Parser::call() {
  auto expr = primary();

  while (true) {
    if (match(TokenType::LEFT_PAREN)) {
      expr = finish_call(std::move(expr));
    } else if (match(TokenType::DOT)) {
      Token& name =
          consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
      expr = std::make_unique<expr::Get>(std::move(expr), name);
    } else {
      break;
    }
  }

  return expr;
}

Scope<Expr> Parser::primary() {
  if (match(TokenType::FALSE)) {
    return std::make_unique<expr::Literal>(false);
  }
  if (match(TokenType::TRUE)) {
    return std::make_unique<expr::Literal>(true);
  }
  if (match(TokenType::NIL)) {
    return std::make_unique<expr::Literal>(Nil{});
  }

  if (match(TokenType::NUMBER)) {
    double value = std::strtod(previous().get_lexeme().c_str(), nullptr);
    return std::make_unique<expr::Literal>(value);
  }
  if (match(TokenType::STRING)) {
    std::string value = previous().get_lexeme();

    // Trim string
    value.pop_back();
    value.erase(value.begin());
    return std::make_unique<expr::Literal>(std::move(value));
  }

  if (match(TokenType::THIS)) {
    return std::make_unique<expr::This>(previous());
  }
  if (match(TokenType::IDENTIFIER)) {
    return std::make_unique<expr::Variable>(previous());
  }

  if (match(TokenType::LEFT_PAREN)) {
    auto expr = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return std::make_unique<expr::Grouping>(std::move(expr));
  }

  throw error(peek(), "Expect expression.");
}

bool Parser::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }

  return false;
}

bool Parser::check(TokenType type) {
  if (is_at_end()) {
    return false;
  }

  return peek().get_type() == type;
}

Token& Parser::advance() {
  if (!is_at_end()) {
    current_++;
  }

  return previous();
}

bool Parser::is_at_end() { return peek().get_type() == TokenType::EOF_; }

Token& Parser::peek() { return tokens_.at(current_); }

Token& Parser::previous() { return tokens_.at(current_ - 1); }

Token& Parser::consume(TokenType type, const std::string& message) {
  if (check(type)) {
    return advance();
  }

  throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
  treewalk::error(token, message);
  return {};
}

void Parser::synchronize() {
  advance();

  while (!is_at_end()) {
    if (previous().get_type() == TokenType::SEMICOLON) {
      return;
    }

    switch (peek().get_type()) {
      case TokenType::CLASS:
      case TokenType::FUN:
      case TokenType::VAR:
      case TokenType::FOR:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::PRINT:
      case TokenType::RETURN:
        return;
      default:
        break;
    }

    advance();
  }
}
}  // namespace lox::treewalk
