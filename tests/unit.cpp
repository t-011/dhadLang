// Unit tests for the tokenizer and parser. Plain C++20 with no framework and
// no nasm, so `make unit` runs them on any OS.

#include <iostream>
#include <string>
#include <vector>

#include "../src/Tokenizer.h"
#include "../src/Parser.h"

using T = TokenType;

static int checks = 0;
static int failures = 0;

static void report(bool ok, int line, const char* what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "tests/unit.cpp:" << line << ": failed: " << what << "\n";
    }
}

#define CHECK(cond) report((cond), __LINE__, #cond)
#define CHECK_TYPES(src, ...) report(types(src) == std::vector<TokenType>{__VA_ARGS__}, __LINE__, #src)

// ---- helpers -------------------------------------------------------------

static std::vector<Token> lex(const std::u32string& src) {
    return Tokenizer(src, src.size()).tokenize();
}

static std::vector<TokenType> types(const std::u32string& src) {
    std::vector<TokenType> out;
    for (const Token& tok : lex(src)) out.push_back(tok.type);
    return out;
}

static std::string opName(TokenType op) {
    switch (op) {
        case T::PLUS: return "+";
        case T::SUB: return "-";
        case T::MULT: return "*";
        case T::DIV: return "/";
        case T::MOD: return "%";
        case T::EQEQ: return "==";
        case T::BANG_EQ: return "!=";
        case T::LS_THAN: return "<";
        case T::GR_THAN: return ">";
        case T::LS_EQ: return "<=";
        case T::GR_EQ: return ">=";
        case T::AND: return "&&";
        case T::OR: return "||";
        case T::BANG: return "!";
        default: return "?";
    }
}

// Prints an expression as an S-expression, e.g. 1 + 2 * 3 -> (+ 1 (* 2 3)),
// so a test states the tree's shape in one string.
static std::string show(const NodeExpr* expr);

static std::string show(const NodeTerm* term) {
    struct Visitor {
        std::string operator()(const NodeTermIntLit* n) const { return n->int_lit.val; }
        std::string operator()(const NodeTermIdent* n) const { return n->ident.val; }
        std::string operator()(const NodeTermParen* n) const { return show(n->expr); }
        std::string operator()(const NodeTermUnary* n) const { return "(" + opName(n->op) + " " + show(n->term) + ")"; }
        std::string operator()(const NodeTermFuncCall* n) const {
            std::string s = "(call " + n->ident.val;
            for (const NodeExpr* arg : n->args) s += " " + show(arg);
            return s + ")";
        }
    };
    return std::visit(Visitor{}, term->var);
}

static std::string show(const NodeExpr* expr) {
    struct Visitor {
        std::string operator()(const NodeTerm* t) const { return show(t); }
        std::string operator()(const BinExpr* b) const {
            return "(" + opName(b->op) + " " + show(b->lhs) + " " + show(b->rhs) + ")";
        }
    };
    return std::visit(Visitor{}, expr->var);
}

// Parses a one-statement program "expr;" and shows its expression.
static std::string exprOf(const std::u32string& src) {
    Parser parser(lex(src));
    NodeProg prog = parser.parseProg();
    return show(std::get<NodeStmtExpr*>(prog.stmts.at(0)->var)->expr);
}

// ---- tokenizer -----------------------------------------------------------

static void testKeywords() {
    CHECK_TYPES(U"دع دالة ارجع اذا واذا والا بينما توقف استمر اطبع خروج",
                T::LET, T::FUNC, T::RETURN, T::IF_, T::ELIF, T::ELSE_, T::WHILE,
                T::BREAK, T::CONTINUE, T::PRINT, T::EXIT);
}

static void testHamzaFolding() {
    CHECK_TYPES(U"إذا وإذا وإلا أرجع", T::IF_, T::ELIF, T::ELSE_, T::RETURN);

    // identifiers keep their spelling: only keyword lookup is folded
    auto tokens = lex(U"أب اب");
    CHECK(tokens.size() == 2);
    CHECK(tokens[0].type == T::IDENT && tokens[0].val == "أب");
    CHECK(tokens[1].type == T::IDENT && tokens[1].val == "اب");
}

static void testNumbers() {
    auto tokens = lex(U"42 ٤٢ ٧0");
    CHECK(tokens.size() == 3);
    CHECK(tokens[0].type == T::INT_LIT && tokens[0].val == "42");
    CHECK(tokens[1].type == T::INT_LIT && tokens[1].val == "42"); // Arabic-Indic digits
    CHECK(tokens[2].type == T::INT_LIT && tokens[2].val == "70"); // mixed digits

    // digits after the first letter belong to the name
    auto ident = lex(U"س١ x2");
    CHECK(ident.size() == 2);
    CHECK(ident[0].type == T::IDENT && ident[0].val == "س١");
    CHECK(ident[1].type == T::IDENT && ident[1].val == "x2");
}

static void testPunctuationAndOperators() {
    CHECK_TYPES(U"، ؛ , ;", T::COMMA, T::SEMI, T::COMMA, T::SEMI);
    CHECK_TYPES(U"== != <= >= < > && || = !",
                T::EQEQ, T::BANG_EQ, T::LS_EQ, T::GR_EQ, T::LS_THAN, T::GR_THAN,
                T::AND, T::OR, T::EQUAL, T::BANG);
    CHECK_TYPES(U"+-*/%(){}", T::PLUS, T::SUB, T::MULT, T::DIV, T::MOD,
                T::OPEN_PAREN, T::CLOSE_PAREN, T::OPEN_CURLY, T::CLOSE_CURLY);
    CHECK_TYPES(U"a<=b", T::IDENT, T::LS_EQ, T::IDENT); // no spaces needed
}

static void testCommentsAndLines() {
    auto tokens = lex(U"1 # ignored ; دع\n# whole line\n\n2");
    CHECK(tokens.size() == 2);
    CHECK(tokens[0].line == 1);
    CHECK(tokens[1].line == 4);
    CHECK(lex(U"# only a comment").empty());
}

// ---- parser --------------------------------------------------------------

static void testPrecedence() {
    CHECK(exprOf(U"1 + 2 * 3;") == "(+ 1 (* 2 3))");
    CHECK(exprOf(U"(1 + 2) * 3;") == "(* (+ 1 2) 3)");
    CHECK(exprOf(U"8 - 3 - 2;") == "(- (- 8 3) 2)"); // left-associative
    CHECK(exprOf(U"1 + 2 == 3;") == "(== (+ 1 2) 3)");
    CHECK(exprOf(U"1 < 2 && 3 >= 4;") == "(&& (< 1 2) (>= 3 4))");
    CHECK(exprOf(U"1 || 0 && 0;") == "(|| 1 (&& 0 0))"); // && binds tighter
    CHECK(exprOf(U"10 % 4 != 2;") == "(!= (% 10 4) 2)");
}

static void testUnary() {
    CHECK(exprOf(U"-2 * 3;") == "(* (- 2) 3)");
    CHECK(exprOf(U"5 - -3;") == "(- 5 (- 3))");
    CHECK(exprOf(U"!ا == 0;") == "(== (! ا) 0)");
    CHECK(exprOf(U"--1;") == "(- (- 1))");
}

static void testCalls() {
    CHECK(exprOf(U"ف();") == "(call ف)");
    CHECK(exprOf(U"ف(1, 2 + 3);") == "(call ف 1 (+ 2 3))");
    CHECK(exprOf(U"ف(٣، ف(٤))؛") == "(call ف 3 (call ف 4))");
}

static void testFunctionDeclaration() {
    Parser parser(lex(U"دالة جمع(أ، ب) {\n    ارجع أ + ب;\n}"));
    NodeProg prog = parser.parseProg();
    CHECK(prog.stmts.size() == 1);

    auto* func = std::get<NodeStmtFuncDecl*>(prog.stmts.at(0)->var);
    CHECK(func->ident.val == "جمع");
    CHECK(func->params.size() == 2);
    CHECK(func->params.at(0).val == "أ" && func->params.at(1).val == "ب");
    CHECK(func->scope->stmts.size() == 1);

    auto* ret = std::get<NodeStmtReturn*>(func->scope->stmts.at(0)->var);
    CHECK(ret->expr.has_value() && show(ret->expr.value()) == "(+ أ ب)");
    CHECK(ret->kw.line == 2);
}

static void testStatements() {
    Parser parser(lex(U"دع ا = 5; ا = ا + 1; ا == 1; خروج(ا); اطبع(ا); دالة ف() { ارجع; }"));
    NodeProg prog = parser.parseProg();
    CHECK(prog.stmts.size() == 6);

    auto* let = std::get<NodeStmtLet*>(prog.stmts.at(0)->var);
    CHECK(let->ident.val == "ا" && show(let->expr) == "5");

    auto* assign = std::get<NodeStmtAssign*>(prog.stmts.at(1)->var);
    CHECK(assign->ident.val == "ا" && show(assign->expr) == "(+ ا 1)");

    // `ا == 1;` is a comparison, not an assignment
    CHECK(show(std::get<NodeStmtExpr*>(prog.stmts.at(2)->var)->expr) == "(== ا 1)");

    CHECK(std::holds_alternative<NodeStmtExit*>(prog.stmts.at(3)->var));
    CHECK(std::holds_alternative<NodeStmtPrint*>(prog.stmts.at(4)->var));

    auto* func = std::get<NodeStmtFuncDecl*>(prog.stmts.at(5)->var);
    CHECK(func->params.empty());
    auto* bareReturn = std::get<NodeStmtReturn*>(func->scope->stmts.at(0)->var);
    CHECK(!bareReturn->expr.has_value());
}

static void testIfChain() {
    Parser parser(lex(U"اذا (1) { } واذا (0) { } والا { خروج(2); }"));
    NodeProg prog = parser.parseProg();
    CHECK(prog.stmts.size() == 1);

    auto* stmtIf = std::get<NodeStmtIf*>(prog.stmts.at(0)->var);
    CHECK(show(stmtIf->expr) == "1");
    CHECK(stmtIf->pred.has_value());

    auto* elif = std::get<NodeIfPredElif*>(stmtIf->pred.value()->var);
    CHECK(show(elif->expr) == "0");
    CHECK(elif->pred.has_value());

    auto* otherwise = std::get<NodeIfPredElse*>(elif->pred.value()->var);
    CHECK(otherwise->scope->stmts.size() == 1);
}

static void testLoopJumps() {
    Parser parser(lex(U"بينما (1) { توقف; استمر; }"));
    NodeProg prog = parser.parseProg();

    auto* loop = std::get<NodeStmtWhile*>(prog.stmts.at(0)->var);
    CHECK(show(loop->expr) == "1");
    CHECK(loop->scope->stmts.size() == 2);
    CHECK(std::get<NodeStmtJump*>(loop->scope->stmts.at(0)->var)->kw.type == T::BREAK);
    CHECK(std::get<NodeStmtJump*>(loop->scope->stmts.at(1)->var)->kw.type == T::CONTINUE);
}

int main() {
    testKeywords();
    testHamzaFolding();
    testNumbers();
    testPunctuationAndOperators();
    testCommentsAndLines();
    testPrecedence();
    testUnary();
    testCalls();
    testFunctionDeclaration();
    testStatements();
    testIfChain();
    testLoopJumps();

    std::cout << "unit: " << checks - failures << "/" << checks << " checks passed" << std::endl;
    return failures == 0 ? 0 : 1;
}
