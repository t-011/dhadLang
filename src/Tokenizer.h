#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <optional>

enum class TokenType {
    ERROR = 0,
    INT_LIT,
    SEMI,
    COMMA,
    EXIT,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_CURLY,
    CLOSE_CURLY,
    IDENT,
    LET,
    RETURN,
    EQUAL,
    EQEQ,
    PLUS,
    SUB,
    MULT,
    DIV,
    MOD,
    IF_,
    ELIF,
    ELSE_,
    WHILE,
    BANG,
    BANG_EQ,
    GR_THAN,
    LS_THAN
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
        while(peek().has_value()) {
            char curr = consume();

            if (isspace(curr)) {
                continue;
            }

            else if (isalpha(curr)) {
                buffer.push_back(curr);
                while (peek() && isalnum(peek().value())) {
                    buffer.push_back(consume());
                }

                if (buffer == "exit") {
                    tokens.push_back({TokenType::EXIT, "exit"});
                }
                else if (buffer == "let") {
                    tokens.push_back({TokenType::LET, "let"});
                }
                else if (buffer == "if") {
                    tokens.push_back({TokenType::IF_, "if"});
                }
                else if (buffer == "elif") { 
                    tokens.push_back({TokenType::ELIF, "elif"});
                }
                else if (buffer == "else") { 
                    tokens.push_back({TokenType::ELSE_, "else"});
                }
                else if (buffer == "while") {
                    tokens.push_back({TokenType::WHILE, "while"});
                }
                else if (buffer == "return") {
                    tokens.push_back({TokenType::RETURN, "return"});
                }
                else {
                    tokens.push_back({TokenType::IDENT, buffer});
                }
                buffer.clear();
            }

            else if (isdigit(curr)) {
                buffer.push_back(curr);
                while (peek() && isdigit(peek().value())) {
                    buffer.push_back(consume());
                }

                tokens.push_back({TokenType::INT_LIT, buffer});
                buffer.clear();
            }

            else if (curr == '=') {
                if (peek() && peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::EQEQ, "=="});
                } else {
                    tokens.push_back({TokenType::EQUAL, "="});
                }
                continue;
            }

            else if (curr == ';') {
                tokens.push_back({TokenType::SEMI, ";"});
                continue;
            }

            else if (curr == '(') {
                tokens.push_back({TokenType::OPEN_PAREN, "("});
                continue;
            }

            else if (curr == ')') {
                tokens.push_back({TokenType::CLOSE_PAREN, ")"});
                continue;
            }

            else if (curr == '{') {
                tokens.push_back({TokenType::OPEN_CURLY, "{"});
                continue;
            }

            else if (curr == '}') {
                tokens.push_back({TokenType::CLOSE_CURLY, "}"});
                continue;
            }

            else if (curr == '+') {
                tokens.push_back({TokenType::PLUS, "+"});
                continue;
            }

            else if (curr == '*') {
                tokens.push_back({TokenType::MULT, "*"});
                continue;
            }

            else if (curr == '-') {
                tokens.push_back({TokenType::SUB, "-"});
                continue;
            }

            else if (curr == '/') {
                tokens.push_back({TokenType::DIV, "/"});
                continue;
            }

            else if (curr == '%') {
                tokens.push_back({TokenType::MOD, "%"});
                continue;
            }

            else if (curr == '!') {
                if (peek() && peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::BANG_EQ, "!="});
                } else {
                    tokens.push_back({TokenType::BANG, "!"});
                }
                continue;
            }

            else if (curr == '>') {
                tokens.push_back({TokenType::GR_THAN, ">"});
                continue;
            }

            else if (curr == '<') {
                tokens.push_back({TokenType::LS_THAN, "<"});
                continue;
            }

            else if (curr == ',') {
                tokens.push_back({TokenType::COMMA, ","});
                continue;
            }

            else {
                std::cerr << "Invalid Syntax" << std::endl;
                exit(1);
            }

        }

        return tokens;
    }

private:
    std::optional<char> peek(unsigned int ahead = 0) const {

        if (m_pos + ahead >= m_src.size()) {
            return {};
        }

        else {
            return m_src.at(m_pos + ahead);
        }
    }

    char consume() {
        return m_src.at(m_pos++);
    }

private:
    std::string m_src;
    size_t m_srcLen;
    size_t m_pos = 0;
};

