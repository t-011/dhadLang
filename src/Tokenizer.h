#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <iostream>
#include <locale>
#include <codecvt>

enum class TokenType {
    ERROR = 0,
    INT_LIT,
    IDENT,
    SEMI,
    COMMA,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_CURLY,
    CLOSE_CURLY,
    // keywords
    EXIT,
    LET,
    RETURN,
    IF_,
    ELIF,
    ELSE_,
    WHILE,
    FUNC,
    PRINT,
    // operators
    EQUAL,
    EQEQ,
    BANG,
    BANG_EQ,
    GR_THAN,
    LS_THAN,
    PLUS,
    SUB,
    MULT,
    DIV,
    MOD
};

struct Token {
    TokenType type;
    std::string val;
    int line = 0;
};

// Every compile error goes through here, so each one names its line.
[[noreturn]] inline void error(int line, const std::string& msg) {
    std::cerr << "line " << line << ": " << msg << std::endl;
    exit(1);
}


class Tokenizer {

public:
    Tokenizer(std::u32string src, size_t len)
        : m_src(std::move(src)), m_srcLen(len) {}


    [[nodiscard]] std::vector<Token> tokenize() {

        static const std::unordered_map<std::u32string, TokenType> keywords = {
            {U"خروج", TokenType::EXIT},
            {U"دع", TokenType::LET},
            {U"اذا", TokenType::IF_},
            {U"واذا", TokenType::ELIF},
            {U"وإلا", TokenType::ELSE_},
            {U"بينما", TokenType::WHILE},
            {U"ارجع", TokenType::RETURN},
            {U"دالة", TokenType::FUNC},
            {U"اطبع", TokenType::PRINT},
        };

        std::vector<Token> tokens;
        auto add = [&](TokenType type, std::string val = "") {
            tokens.push_back({type, std::move(val), m_line});
        };
        // one- or two-character operator, e.g. '<' or '<='
        auto addPair = [&](char32_t next, TokenType pair, TokenType single) {
            if (peek() == next) {
                consume();
                add(pair);
            } else {
                add(single);
            }
        };

        while (peek().has_value()) {
            char32_t curr = consume();

            if (curr == U'\n') {
                m_line++;
            }

            else if (isAsciiSpace(curr)) {
                continue;
            }

            else if (curr == U'#') { // comment until end of line
                while (peek() && peek().value() != U'\n') {
                    consume();
                }
            }

            else if (isAsciiDigit(curr)) {
                std::string num(1, static_cast<char>(curr));
                while (peek() && isAsciiDigit(peek().value())) {
                    num.push_back(static_cast<char>(consume()));
                }
                add(TokenType::INT_LIT, num);
            }

            else if (isAsciiAlpha(curr) || isArabic(curr)) {
                std::u32string word(1, curr);
                while (peek() && (isAsciiAlnum(peek().value()) || isArabic(peek().value()))) {
                    word.push_back(consume());
                }

                auto kw = keywords.find(word);
                if (kw != keywords.end()) {
                    add(kw->second);
                } else {
                    add(TokenType::IDENT, to_utf8(word));
                }
            }

            else {
                switch (curr) {
                    case U';': add(TokenType::SEMI); break;
                    case U',': add(TokenType::COMMA); break;
                    case U'(': add(TokenType::OPEN_PAREN); break;
                    case U')': add(TokenType::CLOSE_PAREN); break;
                    case U'{': add(TokenType::OPEN_CURLY); break;
                    case U'}': add(TokenType::CLOSE_CURLY); break;
                    case U'+': add(TokenType::PLUS); break;
                    case U'-': add(TokenType::SUB); break;
                    case U'*': add(TokenType::MULT); break;
                    case U'/': add(TokenType::DIV); break;
                    case U'%': add(TokenType::MOD); break;
                    case U'=': addPair(U'=', TokenType::EQEQ, TokenType::EQUAL); break;
                    case U'!': addPair(U'=', TokenType::BANG_EQ, TokenType::BANG); break;
                    case U'>': add(TokenType::GR_THAN); break;
                    case U'<': add(TokenType::LS_THAN); break;
                    default:
                        error(m_line, "Invalid character");
                }
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

    static bool isArabic(char32_t c) {
        return c >= 0x0600 && c <= 0x06FF;
    }

    std::string to_utf8(const std::u32string& in) {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        return conv.to_bytes(in);
    }

    static bool isAsciiAlpha(char32_t c) {
        return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
    }

    static bool isAsciiDigit(char32_t c) {
        return c >= U'0' && c <= U'9';
    }

    static bool isAsciiAlnum(char32_t c) {
        return isAsciiAlpha(c) || isAsciiDigit(c);
    }

    static bool isAsciiSpace(char32_t c) {
        return c == U' ' || c == U'\t' || c == U'\r' || c == U'\f' || c == U'\v';
    }

private:
    std::u32string m_src;
    size_t m_srcLen;
    size_t m_pos = 0;
    int m_line = 1;
};
