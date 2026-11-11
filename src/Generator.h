#pragma once

#include <array>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Parser.h"

// Stack layout: _start and every function get an rbp frame. A local is the
// value its `دع` pushed, so it lives at a fixed [rbp - 8*k] no matter what is
// pushed later. Parameters sit above the return address at [rbp + 16 + ...].
// m_stackSize counts slots pushed in the current frame; it only decides
// which k the next local gets.
class Generator {
public:
    Generator(NodeProg root)
        : m_prog(std::move(root)) {}


    void genTerm(const NodeTerm* term) {

        struct TermVisitor {
            Generator* gen;
            void operator()(const NodeTermIntLit* intLit) const {
                gen->out("mov rax, " + intLit->int_lit.val);
                gen->push("rax");
            }

            void operator()(const NodeTermIdent* ident) const {
                gen->push(gen->lookup(ident->ident));
            }

            void operator()(const NodeTermParen* termParen) const {
                gen->genExpr(termParen->expr);
            }

            void operator()(const NodeTermUnary* unary) const {
                gen->genTerm(unary->term);
                gen->pop("rax");
                if (unary->op == TokenType::SUB) {
                    gen->out("neg rax");
                }
                else { // !
                    gen->out("cmp rax, 0");
                    gen->out("sete al");
                    gen->out("movzx rax, al");
                }
                gen->push("rax");
            }

            void operator()(const NodeTermFuncCall* funcCall) const {
                const Token& ident = funcCall->ident;
                auto func = gen->m_funcs.find(ident.val);
                if (func == gen->m_funcs.end()) {
                    error(ident.line, "Function not declared: " + ident.val);
                }
                if (funcCall->args.size() != func->second.paramCount) {
                    error(ident.line, "Wrong number of arguments to " + ident.val);
                }

                for (const auto& arg : funcCall->args) {
                    gen->genExpr(arg);
                }

                gen->out("call " + func->second.label);
                gen->drop(funcCall->args.size());
                gen->push("rax");
            }
        };

        std::visit(TermVisitor{this}, term->var);
    }

    void genBinExpr(const BinExpr* binExpr) {

        if (binExpr->op == TokenType::AND || binExpr->op == TokenType::OR) {
            genShortCircuit(binExpr);
            return;
        }

        genExpr(binExpr->lhs);
        genExpr(binExpr->rhs);

        pop("rbx");
        pop("rax");

        auto compare = [this](const std::string& setcc) {
            out("cmp rax, rbx");
            out(setcc + " al");
            out("movzx rax, al");
        };

        switch (binExpr->op) {
            case TokenType::PLUS:    out("add rax, rbx"); break;
            case TokenType::SUB:     out("sub rax, rbx"); break;
            case TokenType::MULT:    out("imul rax, rbx"); break;
            case TokenType::DIV:     out("cqo"); out("idiv rbx"); break;
            case TokenType::MOD:     out("cqo"); out("idiv rbx"); out("mov rax, rdx"); break;
            case TokenType::EQEQ:    compare("sete"); break;
            case TokenType::BANG_EQ: compare("setne"); break;
            case TokenType::LS_THAN: compare("setl"); break;
            case TokenType::GR_THAN: compare("setg"); break;
            case TokenType::LS_EQ:   compare("setle"); break;
            case TokenType::GR_EQ:   compare("setge"); break;
            default: break; // the parser only builds BinExpr for operators in precedence()
        }

        push("rax");
    }

    // && and || skip the right side once the left side decides the result
    void genShortCircuit(const BinExpr* binExpr) {
        bool isAnd = binExpr->op == TokenType::AND;
        std::string shortLabel = createLabel();
        std::string endLabel = createLabel();

        for (const NodeExpr* side : {binExpr->lhs, binExpr->rhs}) {
            genExpr(side);
            pop("rax");
            out("cmp rax, 0");
            out((isAnd ? "je " : "jne ") + shortLabel);
        }
        out(isAnd ? "mov rax, 1" : "mov rax, 0");
        out("jmp " + endLabel);

        label(shortLabel);
        out(isAnd ? "mov rax, 0" : "mov rax, 1");

        label(endLabel);
        push("rax");
    }

    void genExpr(const NodeExpr* expr) {

        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeTerm* term) const {
                gen->genTerm(term);
            }

            void operator()(const BinExpr* binExpr) const {
                gen->genBinExpr(binExpr);
            }
        };

        std::visit(ExprVisitor{this}, expr->var);
    }

    void genScope(const NodeScope* scope) {
        m_vars.push_scope();

        for (const NodeStmt* stmt : scope->stmts) {
            genStmt(*stmt);
        }

        size_t locals = m_vars.back().size();
        m_vars.pop_scope();
        drop(locals);
    }

    void genIfPred(const NodeIfPred* pred, const std::string& endLabel) {

        struct PredVisitor {
            Generator* gen;
            const std::string& endLabel;

            void operator()(const NodeIfPredElif* predElif) const {
                gen->genExpr(predElif->expr);
                gen->pop("rax");
                gen->out("cmp rax, 0");

                std::string nextLabel = gen->createLabel();
                gen->out("jz " + nextLabel);

                gen->genScope(predElif->scope);
                gen->out("jmp " + endLabel);

                gen->label(nextLabel);

                if (predElif->pred.has_value()) {
                    gen->genIfPred(predElif->pred.value(), endLabel);
                }
            }

            void operator()(const NodeIfPredElse* predElse) const {
                gen->genScope(predElse->scope);
            }
        };

        std::visit(PredVisitor{this, endLabel}, pred->var);
    }

    void genStmt(const NodeStmt& stmt) {

        struct StmtVisitor {
            Generator* gen;

            void operator()(const NodeStmtExit* stmtExit) const {
                gen->genExpr(stmtExit->expr);
                gen->pop("rdi");
                gen->out("mov rax, 60");
                gen->out("syscall");
            }

            void operator()(const NodeStmtPrint* stmtPrint) const {
                gen->genExpr(stmtPrint->expr);
                gen->pop("rax");
                gen->out("call dhad_print");
            }

            void operator()(const NodeStmtLet* stmtLet) const {
                const Token& ident = stmtLet->ident;
                if (gen->m_vars.contains(ident.val)) {
                    error(ident.line, "Identifier already used: " + ident.val);
                }

                gen->genExpr(stmtLet->expr);

                if (gen->m_vars.isGlobal()) {
                    gen->pop("rax");

                    std::string lab = "_g_" + gen->createLabel();
                    gen->m_bss << lab << ":\n    resq 1\n";
                    gen->out("mov QWORD [" + lab + "], rax");

                    gen->m_vars.insert({ident.val, Var{"QWORD [" + lab + "]"}});
                }
                else { // the value just pushed stays on the stack as the variable
                    long slot = static_cast<long>(gen->m_stackSize);
                    gen->m_vars.insert({ident.val, Var{rbpSlot(-8 * slot)}});
                }
            }

            void operator()(const NodeStmtAssign* stmtAssign) const {
                std::string addr = gen->lookup(stmtAssign->ident);
                gen->genExpr(stmtAssign->expr);
                gen->pop("rax");
                gen->out("mov " + addr + ", rax");
            }

            void operator()(const NodeStmtFuncDecl* funcDecl) const {
                const Token& ident = funcDecl->ident;
                if (!gen->m_vars.isGlobal()) {
                    error(ident.line, "Functions must be declared in global scope");
                }
                if (gen->m_funcs.contains(ident.val)) {
                    error(ident.line, "Function already declared: " + ident.val);
                }

                std::string funcLabel = "func" + gen->createLabel();
                gen->m_funcs.insert({ident.val, Func{funcDecl->params.size(), funcLabel}});

                // enter function context
                gen->m_out = &gen->m_funcText;
                gen->m_inFunc = true;
                size_t outerStackSize = gen->m_stackSize;
                gen->m_stackSize = 0;

                gen->label(funcLabel);
                gen->out("push rbp");
                gen->out("mov rbp, rsp");

                gen->m_vars.push_scope();
                size_t paramCount = funcDecl->params.size();
                for (size_t i = 0; i < paramCount; ++i) {
                    const Token& param = funcDecl->params[i];
                    if (gen->m_vars.back().contains(param.val)) {
                        error(param.line, "Parameter already used: " + param.val);
                    }
                    // args are pushed left to right, so the last one is nearest the return address
                    long offset = 16 + 8 * static_cast<long>(paramCount - 1 - i);
                    gen->m_vars.insert({param.val, Var{rbpSlot(offset)}});
                }

                gen->genScope(funcDecl->scope);
                gen->out("mov rax, 0"); // falling off the end returns 0
                gen->leave();

                // exit function context
                gen->m_vars.pop_scope();
                gen->m_stackSize = outerStackSize;
                gen->m_inFunc = false;
                gen->m_out = &gen->m_text;
            }

            void operator()(const NodeStmtReturn* stmtRet) const {
                if (!gen->m_inFunc) {
                    error(stmtRet->kw.line, "Return outside of function");
                }

                if (stmtRet->expr) {
                    gen->genExpr(stmtRet->expr.value());
                    gen->pop("rax");
                }
                else {
                    gen->out("mov rax, 0");
                }

                gen->leave(); // m_stackSize is untouched: code after an early return still uses it
            }

            void operator()(const NodeStmtExpr* stmtExpr) const {
                gen->genExpr(stmtExpr->expr);
                gen->pop("rax"); // discard result
            }

            void operator()(const NodeScope* stmtScope) const {
                gen->genScope(stmtScope);
            }

            void operator()(const NodeStmtIf* stmtIf) const {
                gen->genExpr(stmtIf->expr);
                gen->pop("rax");
                gen->out("cmp rax, 0");

                std::string nextLabel = gen->createLabel();
                gen->out("jz " + nextLabel);

                gen->genScope(stmtIf->scope);

                std::string endLabel = "end" + gen->createLabel();
                gen->out("jmp " + endLabel);

                gen->label(nextLabel);

                if (stmtIf->pred.has_value()) {
                    gen->genIfPred(stmtIf->pred.value(), endLabel);
                }

                gen->label(endLabel);
            }

            void operator()(const NodeStmtWhile* stmtWhile) const {
                std::string startLabel = gen->createLabel();
                std::string endLabel = gen->createLabel();
                gen->label(startLabel);

                gen->genExpr(stmtWhile->expr);
                gen->pop("rax");
                gen->out("cmp rax, 0");
                gen->out("jz " + endLabel);

                gen->genScope(stmtWhile->scope);

                gen->out("jmp " + startLabel);
                gen->label(endLabel);
            }
        };

        std::visit(StmtVisitor{this}, stmt.var);
    }

    [[nodiscard]] std::string genProg() {

        for (const NodeStmt* stmt : m_prog.stmts) {
            genStmt(*stmt);
        }

        std::stringstream asmOut;

        asmOut << "section .bss\n" << m_bss.str() << "\n";

        asmOut << "section .text\n" << PRINT_ROUTINE << "\n" << m_funcText.str() << "\n";

        asmOut << "global _start\n_start:\n"
               << "   mov rbp, rsp\n"
               << m_text.str()
               << "   mov rax, 60\n"
               << "   mov rdi, 0\n"
               << "   syscall\n";

        return asmOut.str();
    }

private:

    // prints rax as a signed decimal plus '\n' with the write syscall (no libc)
    static constexpr const char* PRINT_ROUTINE = R"(dhad_print:
   sub rsp, 32
   lea rsi, [rsp + 31]
   mov byte [rsi], 10
   mov r8, rax
   test rax, rax
   jns .digits
   neg rax
.digits:
   xor rdx, rdx
   mov rcx, 10
   div rcx
   add dl, '0'
   dec rsi
   mov [rsi], dl
   test rax, rax
   jnz .digits
   test r8, r8
   jns .write
   dec rsi
   mov byte [rsi], '-'
.write:
   mov rax, 1
   mov rdi, 1
   lea rdx, [rsp + 32]
   sub rdx, rsi
   syscall
   add rsp, 32
   ret
)";

    void out(const std::string& line) {
        *m_out << "   " << line << "\n";
    }

    void label(const std::string& name) {
        *m_out << name << ":\n";
    }

    void push(const std::string& src) {
        out("push " + src);
        m_stackSize++;
    }

    void pop(const std::string& dst) {
        out("pop " + dst);
        m_stackSize--;
    }

    // discard n slots from the top of the stack
    void drop(size_t n) {
        if (n > 0) {
            out("add rsp, " + std::to_string(n * 8));
        }
        m_stackSize -= n;
    }

    void leave() {
        out("mov rsp, rbp");
        out("pop rbp");
        out("ret");
    }

    std::string createLabel() {
        return "label" + std::to_string(labelCounter++);
    }

    std::string lookup(const Token& ident) {
        if (!m_vars.contains(ident.val)) {
            error(ident.line, "Undeclared identifier: " + ident.val);
        }
        return m_vars.at(ident.val).addr;
    }

    static std::string rbpSlot(long offset) {
        return "QWORD [rbp " + std::string(offset < 0 ? "- " : "+ ") + std::to_string(offset < 0 ? -offset : offset) + "]";
    }

private:

    struct Var {
        std::string addr; // operand for push/mov, e.g. "QWORD [rbp - 8]"
    };

    struct Func {
        size_t paramCount;
        std::string label;
    };

    template<typename T>
    struct ScopeStack {
        std::vector<std::unordered_map<std::string, T>> scopes{1};

        void push_scope() { scopes.push_back({}); }
        void pop_scope() { scopes.pop_back(); }
        std::unordered_map<std::string, T>& back() { return scopes.back(); }

        void insert(const std::pair<std::string, T>& pair) {
            scopes.back().insert(pair);
        }

        bool contains(const std::string& val) const {

            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                if (it->contains(val)) {
                    return true;
                }
            }

            return false;
        }

        T& at(const std::string& val) {

            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                if (it->contains(val)) {
                    return it->at(val);
                }
            }

            throw std::out_of_range("Not found");
        }

        bool isGlobal() {
            return scopes.size() == 1;
        }
    };


    const NodeProg m_prog;

    std::stringstream m_text;     // _start body
    std::stringstream m_funcText; // function bodies
    std::stringstream m_bss;      // globals
    std::stringstream* m_out = &m_text;

    size_t m_stackSize = 0;
    ScopeStack<Var> m_vars{};
    std::unordered_map<std::string, Func> m_funcs;
    bool m_inFunc = false;

    size_t labelCounter = 0;
};
