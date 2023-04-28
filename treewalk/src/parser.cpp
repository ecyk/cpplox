#include "parser.hpp"

#include "treewalk.hpp"

namespace lox::treewalk {
Parser::Parser(std::vector<lox::Token> tokens) : tokens_{std::move(tokens)} {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
  std::vector<std::unique_ptr<Stmt>> statements;
  while (!is_at_end()) {
    statements.push_back(declaration());
  }

  return statements;
}

std::unique_ptr<Stmt> Parser::declaration() {
  try {
    if (match(TOKEN_CLASS)) {
      return class_declaration();
    }
    if (match(TOKEN_FUN)) {
      return fun_declaration();
    }
    if (match(TOKEN_VAR)) {
      return var_declaration();
    }

    return statement();
  } catch (const ParseError&) {
    synchronize();
    return {};
  }
}

std::unique_ptr<Stmt> Parser::class_declaration() {
  const lox::Token& name = consume(TOKEN_IDENTIFIER, "Expect class name.");

  std::optional<expr::Variable> superclass;
  if (match(TOKEN_LESS)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    superclass = expr::Variable{previous()};
  }

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

  std::vector<stmt::Function> methods;
  while (!check(TOKEN_RIGHT_BRACE) && !is_at_end()) {
    methods.push_back(fun_declaration("method"));
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");

  return std::make_unique<stmt::Class>(name, std::move(superclass),
                                       std::move(methods));
}

stmt::Function Parser::fun_declaration(const std::string& kind) {
  const lox::Token& name =
      consume(TOKEN_IDENTIFIER, "Expect " + kind + " name.");
  consume(TOKEN_LEFT_PAREN, "Expect '(' after " + kind + " name.");
  std::vector<lox::Token> parameters;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      constexpr size_t max_parameter_count = 255;
      if (parameters.size() >= max_parameter_count) {
        error(peek(), "Can't have more than 255 parameters.");
      }

      parameters.push_back(consume(TOKEN_IDENTIFIER, "Expect parameter name."));
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

  consume(TOKEN_LEFT_BRACE, "Expect '{' before " + kind + " body.");
  auto body = block();
  return {name, std::move(parameters), std::move(body)};
}

std::unique_ptr<Stmt> Parser::fun_declaration() {
  return std::make_unique<stmt::Function>(fun_declaration("function"));
}

std::unique_ptr<Stmt> Parser::var_declaration() {
  const lox::Token& name = consume(TOKEN_IDENTIFIER, "Expect variable name.");

  std::unique_ptr<Expr> initializer;
  if (match(TOKEN_EQUAL)) {
    initializer = expression();
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  return std::make_unique<stmt::Var>(name, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement() {
  if (match(TOKEN_FOR)) {
    return for_statement();
  }
  if (match(TOKEN_IF)) {
    return if_statement();
  }
  if (match(TOKEN_PRINT)) {
    return print_statement();
  }
  if (match(TOKEN_RETURN)) {
    return return_statement();
  }
  if (match(TOKEN_WHILE)) {
    return while_statement();
  }
  if (match(TOKEN_LEFT_BRACE)) {
    return block_statement();
  }

  return expression_statement();
}

std::unique_ptr<Stmt> Parser::for_statement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  std::unique_ptr<Stmt> initializer;
  if (match(TOKEN_SEMICOLON)) {
  } else if (match(TOKEN_VAR)) {
    initializer = var_declaration();
  } else {
    initializer = expression_statement();
  }

  std::unique_ptr<Expr> condition;
  if (!check(TOKEN_SEMICOLON)) {
    condition = expression();
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

  std::unique_ptr<Expr> increment;
  if (!check(TOKEN_RIGHT_PAREN)) {
    increment = expression();
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
  auto body = statement();

  if (increment) {
    std::vector<std::unique_ptr<Stmt>> loop_statements;
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
    std::vector<std::unique_ptr<Stmt>> block;
    block.push_back(std::move(initializer));
    block.push_back(std::move(body));
    body = std::make_unique<stmt::Block>(std::move(block));
  }

  return body;
}

std::unique_ptr<Stmt> Parser::if_statement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  auto condition = expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

  auto then_branch = statement();
  std::unique_ptr<Stmt> else_branch;
  if (match(TOKEN_ELSE)) {
    else_branch = statement();
  }

  return std::make_unique<stmt::If>(
      std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<Stmt> Parser::print_statement() {
  auto value = expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  return std::make_unique<stmt::Print>(std::move(value));
}

std::unique_ptr<Stmt> Parser::return_statement() {
  const lox::Token& keyword = previous();
  std::unique_ptr<Expr> value;
  if (!check(TOKEN_SEMICOLON)) {
    value = expression();
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
  return std::make_unique<stmt::Return>(keyword, std::move(value));
}

std::unique_ptr<Stmt> Parser::while_statement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  auto condition = expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  auto body = statement();

  return std::make_unique<stmt::While>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::block_statement() {
  return std::make_unique<stmt::Block>(block());
}

std::unique_ptr<Stmt> Parser::expression_statement() {
  auto expr = expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  return std::make_unique<stmt::Expression>(std::move(expr));
}

std::vector<std::unique_ptr<Stmt>> Parser::block() {
  std::vector<std::unique_ptr<Stmt>> statements;

  while (!check(TOKEN_RIGHT_BRACE) && !is_at_end()) {
    statements.push_back(declaration());
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
  return statements;
}

std::unique_ptr<Expr> Parser::finish_call(std::unique_ptr<Expr> callee) {
  std::vector<std::unique_ptr<Expr>> arguments;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      constexpr size_t max_argument_count = 255;
      if (arguments.size() >= max_argument_count) {
        error(peek(), "Can't have more than 255 arguments.");
      }
      arguments.push_back(expression());
    } while (match(TOKEN_COMMA));
  }

  const lox::Token& paren =
      consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

  return std::make_unique<expr::Call>(std::move(callee), paren,
                                      std::move(arguments));
}

std::unique_ptr<Expr> Parser::expression() { return assignment(); }

std::unique_ptr<Expr> Parser::assignment() {
  auto expr = logic_or();

  if (match(TOKEN_EQUAL)) {
    const lox::Token& equals = previous();
    auto value = assignment();

    if (auto* expr_ptr = dynamic_cast<expr::Variable*>(expr.get());
        expr_ptr != nullptr) {
      const lox::Token& name = expr_ptr->name;
      return std::make_unique<expr::Assign>(name, std::move(value));
    }

    if (auto* expr_ptr = dynamic_cast<expr::Get*>(expr.get());
        expr_ptr != nullptr) {
      const lox::Token& name = expr_ptr->name;
      return std::make_unique<expr::Set>(std::move(expr_ptr->object), name,
                                         std::move(value));
    }

    error(equals, "Invalid assignment target.");
  }

  return expr;
}

std::unique_ptr<Expr> Parser::logic_or() {
  auto expr = logic_and();

  while (match(TOKEN_OR)) {
    const lox::Token& op = previous();
    auto right = logic_and();
    expr =
        std::make_unique<expr::Logical>(std::move(expr), op, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::logic_and() {
  auto expr = equality();

  while (match(TOKEN_AND)) {
    const lox::Token& op = previous();
    auto right = equality();
    expr =
        std::make_unique<expr::Logical>(std::move(expr), op, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::equality() {
  auto expr = comparison();

  while (match(TOKEN_BANG_EQUAL) || match(TOKEN_EQUAL_EQUAL)) {
    const lox::Token& op = previous();
    auto right = comparison();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
  auto expr = term();

  while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL) ||
         match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL)) {
    const lox::Token& op = previous();
    auto right = term();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::term() {
  auto expr = factor();

  while (match(TOKEN_MINUS) || match(TOKEN_PLUS)) {
    const lox::Token& op = previous();
    auto right = factor();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::factor() {
  auto expr = unary();

  while (match(TOKEN_SLASH) || match(TOKEN_STAR)) {
    const lox::Token& op = previous();
    auto right = unary();
    expr =
        std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::unary() {
  if (match(TOKEN_BANG) || match(TOKEN_MINUS)) {
    const lox::Token& op = previous();
    auto right = unary();
    return std::make_unique<expr::Unary>(op, std::move(right));
  }

  return call();
}

std::unique_ptr<Expr> Parser::call() {
  auto expr = primary();

  while (true) {
    if (match(TOKEN_LEFT_PAREN)) {
      expr = finish_call(std::move(expr));
    } else if (match(TOKEN_DOT)) {
      const lox::Token& name =
          consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
      expr = std::make_unique<expr::Get>(std::move(expr), name);
    } else {
      break;
    }
  }

  return expr;
}

std::unique_ptr<Expr> Parser::primary() {
  if (match(TOKEN_FALSE)) {
    return std::make_unique<expr::Literal>(false);
  }
  if (match(TOKEN_TRUE)) {
    return std::make_unique<expr::Literal>(true);
  }
  if (match(TOKEN_NIL)) {
    return std::make_unique<expr::Literal>(Value{});
  }

  if (match(TOKEN_NUMBER)) {
    double value = std::strtod(std::string{previous().lexeme}.c_str(), nullptr);
    return std::make_unique<expr::Literal>(value);
  }
  if (match(TOKEN_STRING)) {
    std::string value{previous().lexeme};
    value.pop_back();
    value.erase(value.begin());
    return std::make_unique<expr::Literal>(std::move(value));
  }

  if (match(TOKEN_SUPER)) {
    const lox::Token& keyword = previous();
    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    const lox::Token& method =
        consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    return std::make_unique<expr::Super>(keyword, method);
  }

  if (match(TOKEN_THIS)) {
    return std::make_unique<expr::This>(previous());
  }
  if (match(TOKEN_IDENTIFIER)) {
    return std::make_unique<expr::Variable>(previous());
  }

  if (match(TOKEN_LEFT_PAREN)) {
    auto expr = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
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

  return peek().type == type;
}

lox::Token& Parser::advance() {
  if (!is_at_end()) {
    current_++;
  }

  return previous();
}

bool Parser::is_at_end() { return peek().type == TOKEN_EOF; }

lox::Token& Parser::peek() { return tokens_[current_]; }

lox::Token& Parser::previous() { return tokens_[current_ - 1]; }

lox::Token& Parser::consume(TokenType type, const std::string& message) {
  if (check(type)) {
    return advance();
  }

  throw error(peek(), message);
}

ParseError Parser::error(const lox::Token& token, const std::string& message) {
  if (token.type == TOKEN_ERROR) {
    treewalk::error(token.line, std::string{token.lexeme});
    return {};
  }

  treewalk::error(token, message);
  return {};
}

void Parser::synchronize() {
  advance();

  while (!is_at_end()) {
    if (previous().type == TOKEN_SEMICOLON) {
      return;
    }

    switch (peek().type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:
        break;
    }

    advance();
  }
}
}  // namespace lox::treewalk
