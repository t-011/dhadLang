#pragma once

#include <sstream>
#include <unordered_map>

#include "Parser.h"

class Generator {
public:
    Generator(NodeProg root)
        : m_prog(std::move(root)) {}


    void genExpr(const NodeExpr& expr)  {

        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeExprIntLit& intLit) {
                gen->m_output << "   mov rax, " << intLit.int_lit.val << "\n";
                gen->push("rax");
            }

            void operator()(const NodeExprIdent& ident) {
                if (!gen->m_vars.contains(ident.ident.val)) {
                    std::cerr << "Undeclared Identifier: " << ident.ident.val << std::endl;
                    exit(1);
                }
                const auto& var = gen->m_vars.at(ident.ident.val);
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen->m_stackSize - var.m_stackLoc) * 8 << "]\n";
                gen->push(offset.str());
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

                if (gen->m_vars.contains(stmtLet.ident.val)) {
                    std::cerr << "Identifier already used: " << stmtLet.ident.val << std::endl;
                    exit(1);
                }
                gen->genExpr(stmtLet.expr);
                gen->m_vars.insert({stmtLet.ident.val, Var{ .m_stackLoc = gen->m_stackSize}});

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
        m_output << "   push " << reg << "\n";
        m_stackSize++;
    }

    void pop(const std::string& reg) {
        m_output << "   pop " << reg << "\n";
        m_stackSize--;
    }


private:

    struct Var {
        size_t m_stackLoc;
    };


    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stackSize = 0;
    std::unordered_map<std::string, Var> m_vars{};
};