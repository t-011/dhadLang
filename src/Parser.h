#pragma once

#include <optional>
#include <variant>

#include "Tokenizer.h"
#include <iostream>

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeStmtExit {
    NodeExpr expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit, NodeStmtLet> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};

class Parser {

public:
    Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {}

    std::optional<NodeExpr> parseExpr() {

        if (peek().has_value() && peek().value().type == TokenType::INT_LIT) {
            return NodeExpr{NodeExprIntLit{consume()}};
        }

        else if (peek().has_value() && peek().value().type == TokenType::IDENT) {
            return NodeExpr{NodeExprIdent{consume()}};
        }

        else return {};
    }

    std::optional<NodeStmt> parseStmt() {

        if (peek().has_value() && peek().value().type == TokenType::EXIT) {
            consume();
            NodeStmtExit stmtExit;

            if (peek().has_value() && peek().value().type == TokenType::OPEN_PAREN) {
                consume();
            }
            else {
                std::cerr << "Expected '('" << std::endl;
                exit(1);
            }

            if (auto expr = parseExpr()) {
                stmtExit.expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(1);
            }

            if (peek().has_value() && peek().value().type == TokenType::CLOSE_PAREN) {
                consume();
            }
            else {
                std::cerr << "Expected ')'" << std::endl;
                exit(1);
            }

            if (peek().has_value() && peek().value().type == TokenType::SEMI) {
                consume();
            }
            else {
                std::cerr << "Expected ';'" << std::endl;
                exit(1);
            }
            return NodeStmt{stmtExit};
        }

        if (peek().has_value() && peek().value().type == TokenType::LET) {
            consume();

            NodeStmtLet stmtLet;
            if (peek().has_value() && peek().value().type == TokenType::IDENT) {
                stmtLet.ident = consume();
            }
            else {
                std::cerr << "Expected an identifier" << std::endl;
                exit(1);
            }

            if (peek().has_value() && peek().value().type == TokenType::ASSIGN) {
                consume();
            }
            else {
                std::cerr << "Expected '='" << std::endl;
                exit(1);
            }

            if (auto expr = parseExpr()) {
                stmtLet.expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(1);
            }

            if (peek().has_value() && peek().value().type == TokenType::SEMI) {
                consume();
            }
            else {
                std::cerr << "Expected ';'" << std::endl;
                exit(1);
            }

            return NodeStmt{stmtLet};
        }

        else return {};
    }

    std::optional<NodeProg> parseProg() {
        
        NodeProg prog;
        while (peek().has_value()) {
            if (auto stmt = parseStmt()) {
                prog.stmts.push_back(stmt.value());
            }
            else {
                std::cerr << "Invalid statement" << std::endl;
                exit(1);
            }
        }

        return prog;
    }
    

private:

    std::optional<Token> peek(unsigned int ahead = 0) const {

        if (m_pos + ahead >= m_tokens.size()) {
            return {};
        }

        else {
            return m_tokens.at(m_pos + ahead);
        }
    }

    Token consume() {
        return m_tokens.at(m_pos++);
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos = 0;

};