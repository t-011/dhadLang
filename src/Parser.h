#pragma once

#include <optional>
#include <variant>

#include "Tokenizer.h"
#include "ArenaAlloc.h"
#include <iostream>


struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*> var;
};

struct BinExpr;

struct NodeExpr {
    std::variant<NodeTerm*, BinExpr*> var;
};

struct BinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprMulti {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExpr {
    std::variant<BinExprAdd*, BinExprMulti*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};

class Parser {

public:
    Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
        m_allocator(1024 * 1024 * 4) // 4mb
        {}

    std::optional<BinExpr*> parseBinExpr() {

        if (peek().has_value() && (peek().value().type == TokenType::INT_LIT || peek().value().type == TokenType::IDENT)
            && peek(1).has_value() && peek(1).value().type == TokenType::PLUS
            && peek(2).has_value() && (peek(2).value().type == TokenType::INT_LIT || peek(2).value().type == TokenType::IDENT)) {
            
            BinExpr* binExpr = m_allocator.alloc<BinExpr>();
            BinExprAdd* binExprAdd = m_allocator.alloc<BinExprAdd>();

            if (auto lhsExpr = parseExpr()){
                binExprAdd->lhs = lhsExpr.value();
            }

            consume(); //consume '+'

            if (auto rhsExpr = parseExpr()){
                binExprAdd->rhs = rhsExpr.value();
            }

            binExpr->var = binExprAdd;

            return binExpr;
        }

        else return {};
    }

    std::optional<NodeTerm*> parseTerm() {

        if (peek().has_value() && peek().value().type == TokenType::INT_LIT) {
            NodeTermIntLit* termIntLit = m_allocator.alloc<NodeTermIntLit>();
            termIntLit->int_lit = consume();

            NodeTerm* term = m_allocator.alloc<NodeTerm>();
            term->var = termIntLit;
            return term;
        }

        else if (peek().has_value() && peek().value().type == TokenType::IDENT) {
            NodeTermIdent* termIdent = m_allocator.alloc<NodeTermIdent>();
            termIdent->ident = consume();

            NodeTerm* term = m_allocator.alloc<NodeTerm>();
            term->var = termIdent;
            return term;
        }

        else return {};
    }


    std::optional<NodeExpr*> parseExpr() {

        // auto lhs = parsePrimary();
        // if (!lhs) return {};

        // while (peek() && peek()->type == TokenType::PLUS) {
        //     auto binOperator = consume();

        //     auto rhs = parsePrimary();
        // }
        

        // else if (auto binExpr = parseBinExpr()) {
        //     NodeExpr* expr = m_allocator.alloc<NodeExpr>();
        //     expr->var = binExpr.value();
        //     return expr;
        // }

        // else return {};
    }

    std::optional<NodeStmt> parseStmt() {

        if (peek().has_value() && peek().value().type == TokenType::EXIT) {
            consume();
            
            NodeStmtExit* stmtExit = m_allocator.alloc<NodeStmtExit>();
            if (peek().has_value() && peek().value().type == TokenType::OPEN_PAREN) {
                consume();
            }
            else {
                std::cerr << "Expected '('" << std::endl;
                exit(1);
            }

            if (auto expr = parseExpr()) {
                stmtExit->expr = expr.value();
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

            NodeStmtLet* stmtLet = m_allocator.alloc<NodeStmtLet>();
            if (peek().has_value() && peek().value().type == TokenType::IDENT) {
                stmtLet->ident = consume();
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
                stmtLet->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(1);
            }

            if (peek().has_value() && peek().value().type == TokenType::SEMI) {
                consume();
            }
            else {
                std::cerr << "Expected ';' instead of: " << peek().value().val << std::endl;
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
    ArenaAlloc m_allocator;

};