#pragma once

#include <optional>
#include <variant>

#include "Tokenizer.h"
#include "ArenaAlloc.h"
#include <iostream>

struct NodeScope;
struct NodeExpr;
struct NodeFuncParam {
    Token ident;
};

struct NodeStmtFuncDecl {
    Token ident;
    std::vector<NodeFuncParam*> params;
    NodeScope* scope;
};

struct NodeTermFuncCall {
    Token ident;
    std::vector<NodeExpr*> args;
};

struct NodeStmtReturn {
    NodeExpr* expr;
};

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};


struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*, NodeTermFuncCall*> var;
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

struct BinExprEqTo {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprNotEqTo {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprGrThan {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExprLsThan {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct BinExpr {
    std::variant<BinExprAdd*, BinExprSub*, BinExprDiv*, BinExprMult*, BinExprMod*,
     BinExprEqTo*, BinExprNotEqTo*, BinExprGrThan*, BinExprLsThan*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt;
struct NodeScope {
    std::vector<NodeStmt*> stmts;
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

struct NodeStmtAssign {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtWhile {
    NodeExpr* expr;
    NodeScope* scope;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*, NodeStmtAssign*, NodeStmtWhile*, NodeStmtFuncDecl*, NodeStmtReturn*> var;
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

        if (auto intLit = tryConsume(TokenType::INT_LIT)) {
            NodeTermIntLit* termIntLit = m_allocator.alloc<NodeTermIntLit>();
            termIntLit->int_lit = intLit.value();

            NodeTerm* term = m_allocator.alloc<NodeTerm>();
            term->var = termIntLit;
            return term;
        }

        else if (tryConsume(TokenType::OPEN_PAREN)) {
            auto expr = parseExpr();
            if (!expr) {
                std::cerr << "Expected expression" << std::endl;
                exit(1);
            }

            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");

            auto termParen = m_allocator.alloc<NodeTermParen>();
            termParen->expr = expr.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = termParen;

            return term;
        }

        else if (peek().has_value() && peek().value().type == TokenType::IDENT && 
                 peek(1).has_value() && peek(1).value().type == TokenType::OPEN_PAREN) {

            auto funcCall = m_allocator.alloc<NodeTermFuncCall>();
            funcCall->ident = consume(); consume(); // '('

            while (peek().has_value() && peek().value().type != TokenType::CLOSE_PAREN) {

                if (auto expr = parseExpr()) {
                    funcCall->args.push_back(expr.value());
                }
                else {
                    std::cerr << "Invalid Expression\n";
                    exit(1);
                }
                
                if (peek().has_value() && peek().value().type != TokenType::CLOSE_PAREN) {
                    tryConsumeErr(TokenType::COMMA, "Expected ','");
                }
                
            }
            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = funcCall;
            return term;
        }

        else if (auto ident = tryConsume(TokenType::IDENT)) {
            NodeTermIdent* termIdent = m_allocator.alloc<NodeTermIdent>();
            termIdent->ident = ident.value();

            NodeTerm* term = m_allocator.alloc<NodeTerm>();
            term->var = termIdent;
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
            else if (op.type == TokenType::EQEQ) {
                auto exprEqTo = m_allocator.alloc<BinExprEqTo>();
                exprEqTo->lhs = lhs;
                exprEqTo->rhs = rhs.value();
                binExpr->var = exprEqTo;
            }
            else if (op.type == TokenType::BANG_EQ) {
                auto exprNEqTo = m_allocator.alloc<BinExprNotEqTo>();
                exprNEqTo->lhs = lhs;
                exprNEqTo->rhs = rhs.value();
                binExpr->var = exprNEqTo;
            }
            else if (op.type == TokenType::GR_THAN) {
                auto exprGrThan = m_allocator.alloc<BinExprGrThan>();
                exprGrThan->lhs = lhs;
                exprGrThan->rhs = rhs.value();
                binExpr->var = exprGrThan;
            }
            else if (op.type == TokenType::LS_THAN) {
                auto exprLsThan = m_allocator.alloc<BinExprLsThan>();
                exprLsThan->lhs = lhs;
                exprLsThan->rhs = rhs.value();
                binExpr->var = exprLsThan;
            }
            
            expr->var = binExpr;
            lhs = expr;
        }
        
        return lhs;
    }

    std::optional<NodeScope*> parseScope() {

        if (tryConsumeErr(TokenType::OPEN_CURLY, "Expected '{'")) {

            NodeScope* stmtScope = m_allocator.alloc<NodeScope>();

            while (peek().has_value() && peek().value().type != TokenType::CLOSE_CURLY) {
                if (auto stmt = parseStmt()) {
                    stmtScope->stmts.push_back(stmt.value());
                }
                else {
                    std::cerr << "Invalid statement" << std::endl;
                    exit(1);
                }
            }

            tryConsumeErr(TokenType::CLOSE_CURLY, "Expected '}'");

            return stmtScope;
        }

        else return {};
    }

    std::optional<NodeStmt*> parseStmt() {

        if (tryConsume(TokenType::EXIT)) {
            
            NodeStmtExit* stmtExit = m_allocator.alloc<NodeStmtExit>();

            tryConsumeErr(TokenType::OPEN_PAREN, "Expected '('");

            if (auto expr = parseExpr()) {
                stmtExit->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(1);
            }

            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");

            tryConsumeErr(TokenType::SEMI, "Expected ';'");

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtExit;
            return stmt;
        }

        else if (tryConsume(TokenType::LET)) {

            NodeStmtLet* stmtLet = m_allocator.alloc<NodeStmtLet>();

            auto ident = tryConsumeErr(TokenType::IDENT, "Expected an identifier");
            stmtLet->ident = ident.value();

            tryConsumeErr(TokenType::EQUAL, "Expected '='");

            if (auto expr = parseExpr()) {
                stmtLet->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(1);
            }

            tryConsumeErr(TokenType::SEMI, "Expected ';'");

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtLet;
            return stmt;
        }

        else if (peek().has_value() && peek().value().type == TokenType::IDENT && 
                 peek(1).has_value() && peek(1).value().type == TokenType::EQUAL) {

            auto stmtAssign = m_allocator.alloc<NodeStmtAssign>();
            stmtAssign->ident = consume(); consume(); // '='

            if (auto expr = parseExpr()) {
                stmtAssign->expr = expr.value();
            }

            tryConsumeErr(TokenType::SEMI, "Expected ';'");

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtAssign;
            return stmt;
        }

        else if (peek().has_value() && peek().value().type == TokenType::IDENT && 
                 peek(1).has_value() && peek(1).value().type == TokenType::OPEN_PAREN) {

            auto funcDecl = m_allocator.alloc<NodeStmtFuncDecl>();
            funcDecl->ident = consume(); consume(); // '('

            while (peek().has_value() && peek().value().type != TokenType::CLOSE_PAREN) {
                auto funcParam = m_allocator.alloc<NodeFuncParam>();

                funcParam->ident = tryConsumeErr(TokenType::IDENT, "Expected identifier").value();
                if (peek().has_value() && peek().value().type != TokenType::CLOSE_PAREN) {
                    tryConsumeErr(TokenType::COMMA, "Expected ','");
                }

                funcDecl->params.push_back(funcParam);
            }
            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");
                    
            if (auto scope = parseScope()) {
                funcDecl->scope = scope.value();
            }
            else {
                std::cerr << "Invalid scope\n";
                exit(1);
            }

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = funcDecl;
            return stmt;
        }

        else if (peek().has_value() && peek().value().type == TokenType::OPEN_CURLY) {

            if (auto stmtScope = parseScope()) {
                NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmtScope.value();
                return stmt;
            }

            else {
                std::cout << "Invalid Scope" << std::endl;
                exit(1);
            }
        }

        else if (tryConsume(TokenType::IF_)) {
            
            NodeStmtIf* stmtIf = m_allocator.alloc<NodeStmtIf>();

            tryConsumeErr(TokenType::OPEN_PAREN, "Expected '('");
            
            if (auto expr = parseExpr()) {
                stmtIf->expr = expr.value();
            }
            else {
                std::cerr << "Invalid Expression" << std::endl;
                exit(1);
            }

            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");

            if (auto scope = parseScope()) {
                stmtIf->scope = scope.value();
            }
            else {
                std::cerr << "Invalid Scope" << std::endl;
                exit(1);
            }

            stmtIf->pred = parseIfPred();

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtIf;
            return stmt;
        }

        else if (tryConsume(TokenType::WHILE)) {

            auto stmtWhile = m_allocator.alloc<NodeStmtWhile>();

            tryConsumeErr(TokenType::OPEN_PAREN, "Expected '('");
            
            if (auto expr = parseExpr()) {
                stmtWhile->expr = expr.value();
            }
            else {
                std::cerr << "Invalid Expression" << std::endl;
                exit(1);
            }

            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");

            if (auto scope = parseScope()) {
                stmtWhile->scope = scope.value();
            }
            else {
                std::cerr << "Invalid Scope" << std::endl;
                exit(1);
            }

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtWhile;
            return stmt;
        }

        else if (tryConsume(TokenType::RETURN)) {
            auto stmtRet = m_allocator.alloc<NodeStmtReturn>();

            if (auto expr = parseExpr()) {
                stmtRet->expr = expr.value();
            }
            else {
                std::cerr << "Invalid Expression";
                exit(1);
            }
            tryConsumeErr(TokenType::SEMI, "Expected ';'");

            NodeStmt* stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmtRet;
            return stmt;
        }

        else return {};
    }

    std::optional<NodeIfPred*> parseIfPred() {

        if (tryConsume(TokenType::ELIF)) {
            
            auto ifPredElif = m_allocator.alloc<NodeIfPredElif>();
            
            tryConsumeErr(TokenType::OPEN_PAREN, "Expected '('");
            
            if (auto expr = parseExpr()) {
                ifPredElif->expr = expr.value();
            }
            else {
                std::cerr << "Invalid Expression" << std::endl;
                exit(1);
            }

            tryConsumeErr(TokenType::CLOSE_PAREN, "Expected ')'");

            if (auto scope = parseScope()) {
                ifPredElif->scope = scope.value();
            }
            else {
                std::cerr << "Invalid Scope" << std::endl;
                exit(1);
            }

            ifPredElif->pred = parseIfPred();

            auto ifPred = m_allocator.alloc<NodeIfPred>();
            ifPred->var = ifPredElif;
            return ifPred;
        }

        else if (tryConsume(TokenType::ELSE_)) {

            auto ifPredElse = m_allocator.alloc<NodeIfPredElse>();

            if (auto scope = parseScope()) {
                ifPredElse->scope = scope.value();
            }
            else {
                std::cerr << "Invalid Scope" << std::endl;
                exit(1);
            }

            auto ifPred = m_allocator.alloc<NodeIfPred>();
            ifPred->var = ifPredElse;
            return ifPred;
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

    std::optional<Token> tryConsume(TokenType type) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        else return {};
    }

    std::optional<Token> tryConsumeErr(TokenType type, std::string errMsg) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        else {
            std::cerr << errMsg << std::endl;
            exit(1);
            return {};
        }
    }

    int precedence(TokenType tk) {
        
        switch(tk) {
            case TokenType::PLUS :
            case TokenType::SUB :
                return 3;
            
            case TokenType::MULT :
            case TokenType::DIV :
            case TokenType::MOD :
                return 4;

            case TokenType::EQEQ :
            case TokenType::BANG_EQ :
                return 1;

            case TokenType::GR_THAN :
            case TokenType::LS_THAN :
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