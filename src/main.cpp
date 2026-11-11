#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <stdexcept>

#include "Tokenizer.h"
#include "Parser.h"
#include "Generator.h"

std::u32string readUtf8FileToU32(const std::string& path);

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Too few inputs" << std::endl;
        return 1;
    }

    std::string fileName = argv[1];

    std::u32string contents;
    try {
        contents = readUtf8FileToU32(fileName);
    } catch (const std::range_error&) { // thrown by wstring_convert on bad bytes
        std::cerr << fileName << ": not valid UTF-8" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << fileName << ": " << e.what() << std::endl;
        return 1;
    }

    Tokenizer tk(contents, contents.size());
    auto tokens = tk.tokenize(); 

    Parser p(std::move(tokens));
    NodeProg prog = p.parseProg(); // parse errors exit with a line number

    {
        Generator g(std::move(prog));
        std::ofstream file("out.asm");
        file << g.genProg();
    }

    if (system("nasm -felf64 out.asm") != 0 || system("ld out.o -o out") != 0) {
        std::cerr << "Assembling or linking failed" << std::endl;
        return 1;
    }

    return 0;
}


std::u32string readUtf8FileToU32(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file");

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string utf8 = oss.str();

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.from_bytes(utf8);
}
