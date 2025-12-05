#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>

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

    std::u32string contents = readUtf8FileToU32(fileName);

    Tokenizer tk(contents, contents.size());
    auto tokens = tk.tokenize(); 

    Parser p(std::move(tokens));
    auto prog = p.parseProg();

    if (!prog.has_value()) {
        std::cerr << "Invalid program" << std::endl;
        exit(1);
    }


    {
        Generator g(prog.value());
        std::ofstream file("out.asm");
        file << g.genProg();
    }

    system("nasm -felf64 out.asm");
    system("ld out.o -o out");

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
