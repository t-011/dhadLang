#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "Tokenizer.h"
#include "ArenaAlloc.h"

struct NodeScope;
struct NodeExpr;
struct NodeTerm;

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeTermFuncCall {
    Token ident;
    std::vector<NodeExpr*> args;
};

// -x or !x
struct NodeTermUnary {
    TokenType op;
    NodeTerm* term;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*, NodeTermFuncCall*, NodeTermUnary*> var;
};

// One node for every binary operator; the generator switches on op.
struct BinExpr {
    TokenType op;
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeExpr {
    std::variant<NodeTerm*, BinExpr*> var;
};

struct NodeStmt;
struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtPrint {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtAssign {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtFuncDecl {
    Token ident;
    std::vector<Token> params;
    NodeScope* scope;
};

struct NodeStmtReturn {
    Token kw;
    std::optional<NodeExpr*> expr;
};

// expression whose value is discarded, e.g. a call: f(1);
struct NodeStmtExpr {
    NodeExpr* expr;
};

struct NodeIfPred;
struct NodeIfPredElif {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
    NodeScope* scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElif*, NodeIfPredElse*> var;
};

struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeStmtWhile {
    NodeExpr* expr;
    NodeScope* scope;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtPrint*, NodeStmtLet*, NodeScope*, NodeStmtIf*, NodeStmtAssign*,
                 NodeStmtWhile*, NodeStmtFuncDecl*, NodeStmtReturn*, NodeStmtExpr*> var;
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

        auto term = alloc<NodeTerm>();

        if (auto intLit = tryConsume(TokenType::INT_LIT)) {
            auto termIntLit = alloc<NodeTermIntLit>();
            termIntLit->int_lit = intLit.value();
            term->var = termIntLit;
        }

        else if (peekType() == TokenType::SUB || peekType() == TokenType::BANG) {
            auto unary = alloc<NodeTermUnary>();
            unary->op = consume().type;
            auto inner = parseTerm();
            if (!inner) err("Expected expression");
            unary->term = inner.value();
            term->var = unary;
        }

        else if (tryConsume(TokenType::OPEN_PAREN)) {
            auto termParen = alloc<NodeTermParen>();
            termParen->expr = needExpr();
            expect(TokenType::CLOSE_PAREN, "')'");
            term->var = termParen;
        }

        else if (peekType() == TokenType::IDENT && peekType(1) == TokenType::OPEN_PAREN) {
            auto funcCall = alloc<NodeTermFuncCall>();
            funcCall->ident = consume(); consume(); // '('

            while (peek() && peekType() != TokenType::CLOSE_PAREN) {
                funcCall->args.push_back(needExpr());
                if (peekType() != TokenType::CLOSE_PAREN) {
                    expect(TokenType::COMMA, "','");
                }
            }
            expect(TokenType::CLOSE_PAREN, "')'");
            term->var = funcCall;
        }

        else if (auto ident = tryConsume(TokenType::IDENT)) {
            auto termIdent = alloc<NodeTermIdent>();
            termIdent->ident = ident.value();
            term->var = termIdent;
        }

        else return {};

        return term;
    }


    std::optional<NodeExpr*> parseExpr(int minPrec = 0) {

        auto term = parseTerm();
        if (!term) return {};

        auto lhs = alloc<NodeExpr>();
        lhs->var = term.value();

        while (peek() && precedence(peekType()) >= minPrec) {
            auto op = consume();

            auto rhs = parseExpr(precedence(op.type) + 1);
            if (!rhs) err("Expected expression");

            auto binExpr = alloc<BinExpr>();
            binExpr->op = op.type;
            binExpr->lhs = lhs;
            binExpr->rhs = rhs.value();

            auto expr = alloc<NodeExpr>();
            expr->var = binExpr;
            lhs = expr;
        }

        return lhs;
    }

    NodeScope* parseScope() {

        expect(TokenType::OPEN_CURLY, "'{'");
        auto scope = alloc<NodeScope>();

        while (peek() && peekType() != TokenType::CLOSE_CURLY) {
            auto stmt = parseStmt();
            if (!stmt) err("Invalid statement");
            scope->stmts.push_back(stmt.value());
        }

        expect(TokenType::CLOSE_CURLY, "'}'");
        return scope;
    }

    std::optional<NodeStmt*> parseStmt() {

        auto stmt = alloc<NodeStmt>();

        if (tryConsume(TokenType::EXIT)) {
            auto stmtExit = alloc<NodeStmtExit>();
            stmtExit->expr = parseParenExpr();
            expect(TokenType::SEMI, "';'");
            stmt->var = stmtExit;
        }

        else if (tryConsume(TokenType::PRINT)) {
            auto stmtPrint = alloc<NodeStmtPrint>();
            stmtPrint->expr = parseParenExpr();
            expect(TokenType::SEMI, "';'");
            stmt->var = stmtPrint;
        }

        else if (tryConsume(TokenType::LET)) {
            auto stmtLet = alloc<NodeStmtLet>();
            stmtLet->ident = expect(TokenType::IDENT, "an identifier");
            expect(TokenType::EQUAL, "'='");
            stmtLet->expr = needExpr();
            expect(TokenType::SEMI, "';'");
            stmt->var = stmtLet;
        }

        else if (peekType() == TokenType::IDENT && peekType(1) == TokenType::EQUAL) {
            auto stmtAssign = alloc<NodeStmtAssign>();
            stmtAssign->ident = consume(); consume(); // '='
            stmtAssign->expr = needExpr();
            expect(TokenType::SEMI, "';'");
            stmt->var = stmtAssign;
        }

        else if (tryConsume(TokenType::FUNC)) {
            auto funcDecl = alloc<NodeStmtFuncDecl>();
            funcDecl->ident = expect(TokenType::IDENT, "function name");
            expect(TokenType::OPEN_PAREN, "'('");

            while (peek() && peekType() != TokenType::CLOSE_PAREN) {
                funcDecl->params.push_back(expect(TokenType::IDENT, "parameter name"));
                if (peekType() != TokenType::CLOSE_PAREN) {
                    expect(TokenType::COMMA, "','");
                }
            }
            expect(TokenType::CLOSE_PAREN, "')'");

            funcDecl->scope = parseScope();
            stmt->var = funcDecl;
        }

        else if (peekType() == TokenType::OPEN_CURLY) {
            stmt->var = parseScope();
        }

        else if (tryConsume(TokenType::IF_)) {
            auto stmtIf = alloc<NodeStmtIf>();
            stmtIf->expr = parseParenExpr();
            stmtIf->scope = parseScope();
            stmtIf->pred = parseIfPred();
            stmt->var = stmtIf;
        }

        else if (tryConsume(TokenType::WHILE)) {
            auto stmtWhile = alloc<NodeStmtWhile>();
            stmtWhile->expr = parseParenExpr();
            stmtWhile->scope = parseScope();
            stmt->var = stmtWhile;
        }

        else if (auto kw = tryConsume(TokenType::RETURN)) {
            auto stmtRet = alloc<NodeStmtReturn>();
            stmtRet->kw = kw.value();
            if (peekType() != TokenType::SEMI) {
                stmtRet->expr = needExpr();
            }
            expect(TokenType::SEMI, "';'");
            stmt->var = stmtRet;
        }

        else if (auto expr = parseExpr()) {
            auto stmtExpr = alloc<NodeStmtExpr>();
            stmtExpr->expr = expr.value();
            expect(TokenType::SEMI, "';'");
            stmt->var = stmtExpr;
        }

        else return {};

        return stmt;
    }

    std::optional<NodeIfPred*> parseIfPred() {

        auto ifPred = alloc<NodeIfPred>();

        if (tryConsume(TokenType::ELIF)) {
            auto ifPredElif = alloc<NodeIfPredElif>();
            ifPredElif->expr = parseParenExpr();
            ifPredElif->scope = parseScope();
            ifPredElif->pred = parseIfPred();
            ifPred->var = ifPredElif;
        }

        else if (tryConsume(TokenType::ELSE_)) {
            auto ifPredElse = alloc<NodeIfPredElse>();
            ifPredElse->scope = parseScope();
            ifPred->var = ifPredElse;
        }

        else return {};

        return ifPred;
    }

    NodeProg parseProg() {

        NodeProg prog;
        while (peek()) {
            auto stmt = parseStmt();
            if (!stmt) err("Invalid statement");
            prog.stmts.push_back(stmt.value());
        }

        return prog;
    }


private:

    template<typename T>
    T* alloc() {
        return m_allocator.alloc<T>();
    }

    std::optional<Token> peek(size_t ahead = 0) const {
        if (m_pos + ahead >= m_tokens.size()) {
            return {};
        }
        return m_tokens.at(m_pos + ahead);
    }

    // ERROR when past the end, so comparisons need no has_value() check
    TokenType peekType(size_t ahead = 0) const {
        auto tok = peek(ahead);
        return tok ? tok->type : TokenType::ERROR;
    }

    Token consume() {
        return m_tokens.at(m_pos++);
    }

    std::optional<Token> tryConsume(TokenType type) {
        if (peekType() == type) {
            return consume();
        }
        return {};
    }

    // "Expected X" points at the last good token, i.e. where X is missing
    Token expect(TokenType type, const std::string& what) {
        if (peekType() == type) {
            return consume();
        }
        error(m_pos > 0 ? m_tokens.at(m_pos - 1).line : 1, "Expected " + what);
    }

    [[noreturn]] void err(const std::string& msg) const {
        int line = 1;
        if (peek()) line = peek()->line;
        else if (!m_tokens.empty()) line = m_tokens.back().line;
        error(line, msg);
    }

    NodeExpr* needExpr() {
        auto expr = parseExpr();
        if (!expr) err("Expected expression");
        return expr.value();
    }

    // ( expr )
    NodeExpr* parseParenExpr() {
        expect(TokenType::OPEN_PAREN, "'('");
        auto expr = needExpr();
        expect(TokenType::CLOSE_PAREN, "')'");
        return expr;
    }

    static int precedence(TokenType tk) {

        switch(tk) {
            case TokenType::EQEQ :
            case TokenType::BANG_EQ :
                return 1;

            case TokenType::GR_THAN :
            case TokenType::LS_THAN :
                return 2;

            case TokenType::PLUS :
            case TokenType::SUB :
                return 3;

            case TokenType::MULT :
            case TokenType::DIV :
            case TokenType::MOD :
                return 4;

            default:
                return -1;
        }
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
    ArenaAlloc m_allocator;

};
