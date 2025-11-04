#pragma once

#include <sstream>

#include "Parser.h"

class Generator {
public:
    Generator(NodeExit root)
        : m_root(std::move(root)) {}


    [[nodiscard]] std::string generate() const {

        std::stringstream output;
        output << "global _start\n_start:\n";
        output << "   mov rax, 60\n";
        output << "   mov rdi, " << m_root.expr.token.val << "\n";
        output << "   syscall\n";
        return output.str();
    }



private:
    NodeExit m_root;
};