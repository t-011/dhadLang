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

struct NodeExpr;
struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};


struct BinExpr;
struct NodeExpr {
    std::variant<NodeTerm*, BinExpr*> var;
};

struct BinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprMult {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprMod {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExpr {
    std::variant<BinExprAdd*, BinExprSub*, BinExprDiv*, BinExprMult*, BinExprMod*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt;
struct NodeStmtScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeStmtScope*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {

public:
    Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
        m_allocator(1024 * 1024 * 4) // 4mb
        {}

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

        else if (peek().has_value() && peek().value().type == TokenType::OPEN_PAREN) {
            consume();
            auto expr = parseExpr();
            if (!expr) {
                std::cerr << "Expected expression" << std::endl;
                exit(1);
            }
            if (peek().has_value() && peek().value().type == TokenType::CLOSE_PAREN) {
                consume();
            }
            else {
                std::cerr << "Expected ')'" << std::endl;
                exit(1);
            }

            auto termParen = m_allocator.alloc<NodeTermParen>();
            termParen->expr = expr.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = termParen;

            return term;
        }

        else return {};
    }


    std::optional<NodeExpr*> parseExpr(int minPrec = 0) {

        NodeExpr* lhs = m_allocator.alloc<NodeExpr>();

        auto term = parseTerm();
        if (!term) return {};
        lhs->var = term.value();

        while (peek() && precedence(peek()->type) >= minPrec) {
            auto op = consume();
            auto rhs = parseExpr(precedence(op.type) + 1);
            if (!rhs) {
                std::cerr << "Expected RHS" << std::endl;
                exit(1);
            }

            auto binExpr = m_allocator.alloc<BinExpr>();
            auto expr = m_allocator.alloc<NodeExpr>();

            if (op.type == TokenType::PLUS) {
                auto exprAdd = m_allocator.alloc<BinExprAdd>();
                exprAdd->lhs = lhs;
                exprAdd->rhs = rhs.value();
                binExpr->var = exprAdd;
            }
            else if (op.type == TokenType::SUB) {
                auto exprSub = m_allocator.alloc<BinExprSub>();
                exprSub->lhs = lhs;
                exprSub->rhs = rhs.value();
                binExpr->var = exprSub;
            }
            else if (op.type == TokenType::MULT) {
                auto exprMult = m_allocator.alloc<BinExprMult>();
                exprMult->lhs = lhs;
                exprMult->rhs = rhs.value();
                binExpr->var = exprMult;
            }
            else if (op.type == TokenType::DIV) {
                auto exprDiv = m_allocator.alloc<BinExprDiv>();
                exprDiv->lhs = lhs;
                exprDiv->rhs = rhs.value();
                binExpr->var = exprDiv;
            }
            else if (op.type == TokenType::MOD) {
                auto exprMod = m_allocator.alloc<BinExprMod>();
                exprMod->lhs = lhs;
                exprMod->rhs = rhs.value();
                binExpr->var = exprMod;
            }

            expr->var = binExpr;
            lhs = expr;
        }
        
        return lhs;
    }

    std::optional<NodeStmt*> parseStmt() {

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
            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtExit;
            return stmt;
        }

        else if (peek().has_value() && peek().value().type == TokenType::LET) {
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

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtLet;
            return stmt;
        }

        else if (peek().has_value() && peek().value().type == TokenType::OPEN_CURLY) {
            consume();

            NodeStmtScope* stmtScope = m_allocator.alloc<NodeStmtScope>();

            while (peek().has_value() && peek().value().type != TokenType::CLOSE_CURLY) {
                if (auto stmt = parseStmt()) {
                    stmtScope->stmts.push_back(stmt.value());
                }
                else {
                    std::cerr << "Invalid statement" << std::endl;
                    exit(1);
                }
            }

            if (!peek().has_value()) {
                std::cerr << "Expected '}'" << std::endl;
                exit(1);
            }
            consume(); //'}'

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtScope;
            return stmt;
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

    int precedence(TokenType tk) {
        
        switch(tk) {
            case TokenType::PLUS :
                return 1;

            case TokenType::SUB :
                return 1;
            
            case TokenType::MULT :
                return 2;

            case TokenType::DIV :
                return 2;
            
            case TokenType::MOD :
                return 2;

            default:
                return -1;
        }
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
    ArenaAlloc m_allocator;

};