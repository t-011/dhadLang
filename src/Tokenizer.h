#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <optional>
#include <locale>
#include <codecvt>

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
    Tokenizer(std::u32string src, size_t len)
        : m_src(std::move(src)), m_srcLen(len) {}


    [[nodiscard]] std::vector<Token> tokenize() {
        
        std::vector<Token> tokens;

        std::u32string buffer;
        while(peek().has_value()) {
            char32_t curr = consume();

            if (isAsciiSpace(curr)) {
                continue;
            }

            else if (isAsciiAlpha(curr) || isArabic(curr)) {
                buffer.push_back(curr);
                while (peek() && (isAsciiAlnum(peek().value()) || isArabic(peek().value()))) {
                    buffer.push_back(consume());
                }

                if (buffer == U"خروج") {
                    tokens.push_back({TokenType::EXIT});
                }
                else if (buffer == U"دع") {
                    tokens.push_back({TokenType::LET});
                }
                else if (buffer == U"اذا") {
                    tokens.push_back({TokenType::IF_});
                }
                else if (buffer == U"واذا") { 
                    tokens.push_back({TokenType::ELIF});
                }
                else if (buffer == U"وإلا") { 
                    tokens.push_back({TokenType::ELSE_});
                }
                else if (buffer == U"بينما") {
                    tokens.push_back({TokenType::WHILE});
                }
                else if (buffer == U"ارجع") {
                    tokens.push_back({TokenType::RETURN});
                }
                else {
                    tokens.push_back({TokenType::IDENT, to_utf8(buffer)});
                }
                buffer.clear();
            }

            else if (isAsciiDigit(curr)) {
                buffer.push_back(curr);
                while (peek() && isAsciiDigit(peek().value())) {
                    buffer.push_back(consume());
                }

                tokens.push_back({TokenType::INT_LIT, to_utf8(buffer)});
                buffer.clear();
            }

            else if (curr == '=') {
                if (peek() && peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::EQEQ});
                } else {
                    tokens.push_back({TokenType::EQUAL});
                }
            }

            else if (curr == ';') {
                tokens.push_back({TokenType::SEMI});
            }

            else if (curr == '(') {
                tokens.push_back({TokenType::OPEN_PAREN});
            }

            else if (curr == ')') {
                tokens.push_back({TokenType::CLOSE_PAREN});
            }

            else if (curr == '{') {
                tokens.push_back({TokenType::OPEN_CURLY});
            }

            else if (curr == '}') {
                tokens.push_back({TokenType::CLOSE_CURLY});
            }

            else if (curr == '+') {
                tokens.push_back({TokenType::PLUS});
            }

            else if (curr == '*') {
                tokens.push_back({TokenType::MULT});
            }

            else if (curr == '-') {
                tokens.push_back({TokenType::SUB});
            }

            else if (curr == '/') {
                tokens.push_back({TokenType::DIV});
            }

            else if (curr == '%') {
                tokens.push_back({TokenType::MOD});
            }

            else if (curr == '!') {
                if (peek() && peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::BANG_EQ});
                } else {
                    tokens.push_back({TokenType::BANG});
                }
            }

            else if (curr == '>') {
                tokens.push_back({TokenType::GR_THAN});
            }

            else if (curr == '<') {
                tokens.push_back({TokenType::LS_THAN});
            }

            else if (curr == ',') {
                tokens.push_back({TokenType::COMMA});
            }

            else {
                std::cerr << "Invalid Syntax" << std::endl;
                exit(1);
            }

        }

        return tokens;
    }

private:
    std::optional<char32_t> peek(unsigned int ahead = 0) const {

        if (m_pos + ahead >= m_src.size()) {
            return {};
        }

        else {
            return m_src.at(m_pos + ahead);
        }
    }

    char32_t consume() {
        return m_src.at(m_pos++);
    }

    bool isArabic(char32_t c) {
        return (c >= 0x0600 && c <= 0x06FF);
    }

    std::string to_utf8(const std::u32string& in) {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        return conv.to_bytes(in);
    }

    bool isAsciiAlpha(char32_t c) {
        return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
    }

    bool isAsciiDigit(char32_t c) {
        return (c >= U'0' && c <= U'9');
    }

    bool isAsciiAlnum(char32_t c) {
        return isAsciiAlpha(c) || isAsciiDigit(c);
    }

    bool isAsciiSpace(char32_t c) {
        return c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == U'\f' || c == U'\v';
    }

private:
    std::u32string m_src;
    size_t m_srcLen;
    size_t m_pos = 0;
};

