#pragma once

#include <sstream>
#include <unordered_map>
#include <variant>

#include "Parser.h"

class Generator {
public:
    Generator(NodeProg root)
        : m_prog(std::move(root)) {}


    void genTerm(const NodeTerm* term) {

        struct TermVisitor {
            Generator* gen;
            void operator()(const NodeTermIntLit* intLit) const {
                gen->m_output << "   mov rax, " << intLit->int_lit.val << "\n";
                gen->push("rax");
            }

            void operator()(const NodeTermIdent* ident) const {
                if (!gen->m_vars.contains(ident->ident.val)) {
                    std::cerr << "Undeclared Identifier: " << ident->ident.val << std::endl;
                    exit(1);
                }
                const auto& var = gen->m_vars.at(ident->ident.val);
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen->m_stackSize - var.m_stackLoc) * 8 << "]";
                gen->push(offset.str());
            }

            void operator()(const NodeTermParen* termParen) const {
                gen->genExpr(termParen->expr);
            }
        };

        TermVisitor visitor{this};
        std::visit(visitor, term->var);
    }

    void genBinExpr(const BinExpr* binExpr) {

        struct BinExprVisitor {
            Generator* gen;
            void operator()(const BinExprAdd* exprAdd) {
                gen->genExpr(exprAdd->lhs);
                gen->genExpr(exprAdd->rhs);

                gen->pop("rbx");
                gen->pop("rax");

                gen->m_output << "   add rax, rbx\n";
                
                gen->push("rax");
            }

            void operator()(const BinExprSub* exprSub) {
                gen->genExpr(exprSub->lhs);
                gen->genExpr(exprSub->rhs);

                gen->pop("rbx");
                gen->pop("rax");

                gen->m_output << "   sub rax, rbx\n";
                
                gen->push("rax");
            }

            void operator()(const BinExprMult* exprMulti) {
                gen->genExpr(exprMulti->lhs);
                gen->genExpr(exprMulti->rhs);

                gen->pop("rbx");
                gen->pop("rax");

                gen->m_output << "   imul rax, rbx\n";
                
                gen->push("rax");
            }

            void operator()(const BinExprDiv* exprDiv) {
                gen->genExpr(exprDiv->lhs);
                gen->genExpr(exprDiv->rhs);

                gen->pop("rbx");
                gen->pop("rax");

                gen->m_output << "   cqo\n";
                gen->m_output << "   idiv rbx\n";
                
                gen->push("rax");
            }

            void operator()(const BinExprMod* exprMod) {
                gen->genExpr(exprMod->lhs);
                gen->genExpr(exprMod->rhs);

                gen->pop("rbx");
                gen->pop("rax");

                gen->m_output << "   idiv rbx\n";
                
                gen->push("rdx");
            }
        };

        BinExprVisitor visitor{this};
        std::visit(visitor, binExpr->var);
    }

    void genExpr(const NodeExpr* expr)  {

        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeTerm* term) {
                gen->genTerm(term);
            }

            void operator()(const BinExpr* binExpr) {
                gen->genBinExpr(binExpr);
            }
        };

        ExprVisitor visitor{this};
        std::visit(visitor, expr->var);
    }

    void genScope(const NodeScope* scope) {
        scopeBegin();

        for (const NodeStmt* stmt : scope->stmts) {
            genStmt(*stmt);
        }

        scopeEnd();
    }

    void genIfPred(const NodeIfPred* pred, std::string endLabel) {

        struct PredVisitor {
            Generator* gen;
            std::string endLabel;

            void operator()(const NodeIfPredElif* predElif) {
                gen->genExpr(predElif->expr);
                gen->pop("rax");
                gen->m_output << "   cmp rax, 0\n";

                std::string label = gen->createLabel();
                gen->m_output << "   jz " << label << "\n";
                
                gen->genScope(predElif->scope);

                gen->m_output << "   jmp " << endLabel << "\n";

                gen->m_output << label << ":\n";

                if (predElif->pred.has_value()) {
                    gen->genIfPred(predElif->pred.value(), endLabel);
                }
            }
            void operator()(const NodeIfPredElse* predElse) {
                gen->genScope(predElse->scope);
            }
        };

        PredVisitor visitor{this, endLabel};
        std::visit(visitor, pred->var);
    }

    void genStmt(const NodeStmt& stmt) {

        struct StmtVisitor {
            Generator* gen;
            void operator()(const NodeStmtExit* stmtExit) {
                gen->genExpr(stmtExit->expr);

                gen->m_output << "   mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "   syscall\n";
            }

            void operator()(const NodeStmtLet* stmtLet) {

                if (gen->m_vars.contains(stmtLet->ident.val)) {
                    std::cerr << "Identifier already used: " << stmtLet->ident.val << std::endl;
                    exit(1);
                }
                gen->genExpr(stmtLet->expr);
                gen->m_vars.insert({stmtLet->ident.val, Var{ .m_stackLoc = gen->m_stackSize}});

            }

            void operator()(const NodeScope* stmtScope) {
                gen->genScope(stmtScope);
            }

            void operator()(const NodeStmtIf* stmtIf) {
                
                gen->genExpr(stmtIf->expr);
                gen->pop("rax");
                gen->m_output << "   cmp rax, 0\n";

                std::string label = gen->createLabel();
                gen->m_output << "   jz " << label << "\n";
                
                gen->genScope(stmtIf->scope);

                std::string endLabel = "end" + gen->createLabel();
                gen->m_output << "   jmp " << endLabel << "\n";

                gen->m_output << label << ":\n";

                if (stmtIf->pred.has_value()) {
                    gen->genIfPred(stmtIf->pred.value(), endLabel);
                }

                gen->m_output << endLabel << ":\n";
            }
        };

        StmtVisitor visitor{this};
        std::visit(visitor, stmt.var);
    }

    [[nodiscard]] std::string genProg() {

        m_output << "global _start\n_start:\n";

        for (const NodeStmt* stmt : m_prog.stmts) {
            genStmt(*stmt);
        }

        m_output << "   mov rax, 60\n";
        m_output << "   mov rdi, 0\n";
        m_output << "   syscall\n";

        return m_output.str();
    }

private:

    void push(const std::string& reg) {
        m_output << "   push " << reg << "\n";
        m_stackSize++;
    }

    void pop(const std::string& reg) {
        m_output << "   pop " << reg << "\n";
        m_stackSize--;
    }

    void scopeBegin() {
        m_vars.push_scope();
    }

    void scopeEnd() {
        size_t scopeSize = m_vars.back().size();
        m_vars.pop_scope();
        m_output << "   add rsp, " << (scopeSize * 8) << "\n";
        m_stackSize -= scopeSize;
    }

    std::string createLabel() {
        return "label" + std::to_string(labelCounter++);
    }


private:

    struct Var {
        size_t m_stackLoc;
    };

    struct ScopeStack {
        std::vector<std::unordered_map<std::string, Var>> scopes{1};

        void push_scope() { scopes.push_back({}); }
        void pop_scope() { scopes.pop_back(); }
        std::unordered_map<std::string, Var>& back() { return scopes.back(); }
        
        void insert(const std::pair<std::string, Var>& pair) {
            scopes.back().insert(pair);
        }

        const bool contains(const std::string& val) const {

            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                if (it->contains(val)) {
                    return true;
                }
            }

            return false;
        }

        const Var& at(const std::string& val) const {

            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                if (it->contains(val)) {
                    return it->at(val);
                }
            }

            throw std::out_of_range("Variable not found");
        }
    };


    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stackSize = 0;
    ScopeStack m_vars{};
    size_t labelCounter = 0;
};