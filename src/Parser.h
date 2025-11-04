#pragma once

#include <optional>

#include "Tokenizer.h"
#include <iostream>

struct NodeExpr {
    Token token;
};

struct NodeExit {
    NodeExpr expr;
};


class Parser {

public:
    Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {}

    [[nodiscard]] std::optional<NodeExpr> parseExpr() {

        if (peek().has_value() && peek().value().type == TokenType::INT_LIT) {
            return NodeExpr{consume()};
        }

        else return {};
    }

    std::optional<NodeExit> parse() {
        
        std::optional<NodeExit> nodeExit;

        while (peek().has_value()) {
            auto val = consume();

            if (val.type == TokenType::EXIT) {

                if (auto expr = parseExpr()) {
                    nodeExit = NodeExit{expr.value()};
                }
                else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(1);
                }

                if (peek().has_value() && peek().value().type == TokenType::SEMI) {
                    consume();
                }
                else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(1);
                }

            }
        }

        return nodeExit;
    }
    

private:

    std::optional<Token> peek(unsigned int ahead = 0) const {

        if (m_pos + ahead >= m_tokens.size()) {
            return {};
        }

        else {
            return m_tokens.at(m_pos);
        }
    }

    Token consume() {
        return m_tokens.at(m_pos++);
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos = 0;

};