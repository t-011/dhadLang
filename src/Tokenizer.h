#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cctype>

enum class TokenType {
    ERROR = 0,
    INT_LIT,
    SEMI,
    EXIT,
    OPEN_PAREN,
    CLOSE_PAREN,
    IDENT,
    LET,
    ASSIGN,
    PLUS,
    MULTI
};

struct Token {
    TokenType type;
    std::string val;
};


class Tokenizer {

public:
    Tokenizer(std::string src, size_t len)
        : m_src(std::move(src)), m_srcLen(len) {}


    [[nodiscard]] std::vector<Token> tokenize() {
        
        std::vector<Token> tokens;

        std::string buffer;
        for (size_t i = 0; i < m_srcLen; ++i) {
            char curr = m_src[i];

            if (isspace(curr)) {
                continue;
            }

            if (isalpha(curr)) {
                while (i < m_srcLen && isalnum(m_src[i])) {
                    buffer.push_back(m_src[i]);
                    ++i;
                }

                if (i >= m_srcLen) {
                    //ERROR (cause semi needs to be last)
                    tokens.clear();
                    return tokens;
                }

                --i; // while loop, off by one
                if (buffer == "exit") {
                    tokens.push_back({TokenType::EXIT, "exit"});
                }
                else if (buffer == "let") {
                    tokens.push_back({TokenType::LET, "let"});
                } 
                else {
                    tokens.push_back({TokenType::IDENT, buffer});
                }
                buffer.clear();
                continue;
            }

            if (isdigit(curr)) {
                while (i < m_srcLen && isdigit(m_src[i])) {
                    buffer.push_back(m_src[i]);
                    ++i;
                }

                if (i >= m_srcLen) {
                    //ERROR (cause semi needs to be last)
                    tokens.clear();
                    return tokens;
                }

                --i;
                tokens.push_back({TokenType::INT_LIT, buffer});
                buffer.clear();
                continue;
            }

            if (curr == '=') {
                tokens.push_back({TokenType::ASSIGN, "="});
                continue;
            }

            if (curr == ';') {
                tokens.push_back({TokenType::SEMI, ";"});
                continue;
            }

            if (curr == '(') {
                tokens.push_back({TokenType::OPEN_PAREN, "("});
                continue;
            }

            if (curr == ')') {
                tokens.push_back({TokenType::CLOSE_PAREN, ")"});
                continue;
            }

            if (curr == '+') {
                tokens.push_back({TokenType::PLUS, "+"});
                continue;
            }

        }

        return (!tokens.empty() && tokens.back().type == TokenType::SEMI) ? tokens : std::vector<Token>{};
    }

private:
    std::string m_src;
    size_t m_srcLen;

};

