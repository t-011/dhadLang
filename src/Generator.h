#pragma once

#include <sstream>

#include "Parser.h"

class Generator {
public:
    Generator(NodeProg root)
        : m_prog(std::move(root)) {}


    void genExpr(const NodeExpr& expr)  {

        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeExprIntLit& intLit) {
                gen->m_output << "    mov rax, " << intLit.int_lit.val << "\n";
                gen->push("rax");
            }

            void operator()(const NodeExprIdent& ident) {

            }
        };

        ExprVisitor visitor{this};
        std::visit(visitor, expr.var);
    }


    void genStmt(const NodeStmt& stmt) {

        struct StmtVisitor {
            Generator* gen;
            void operator()(const NodeStmtExit& stmtExit) {
                gen->genExpr(stmtExit.expr);

                gen->m_output << "   mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "   syscall\n";
            }

            void operator()(const NodeStmtLet& stmtLet) {

            }
        };

        StmtVisitor visitor{this};
        std::visit(visitor, stmt.var);
    }

    [[nodiscard]] std::string genProg() {

        m_output << "global _start\n_start:\n";

        for (const NodeStmt& stmt : m_prog.stmts) {
            genStmt(stmt);
        }

        m_output << "   mov rax, 60\n";
        m_output << "   mov rdi, 0\n";
        m_output << "   syscall\n";

        return m_output.str();
    }

private:

    void push(const std::string& reg) {
        m_output << "push " << reg << "\n";
        m_stackSize++;
    }

    void pop(const std::string& reg) {
        m_output << "pop " << reg << "\n";
        m_stackSize--;
    }


private:
    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stackSize = 0;
    // std::unordered_map<std::string, Var> m_vars;
};