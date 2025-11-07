#include <iostream>
#include <sstream>
#include <fstream>

#include "Tokenizer.h"
#include "Parser.h"
#include "Generator.h"


int main(int argc, char* argv[]) {

    // if (argc < 2) {
    //     std::cerr << "Too few inputs" << std::endl;
    //     return 1;
    // }

    std::string contents;
    
    {
        std::ifstream file("main.dhad");
        std::stringstream buf;
        buf << file.rdbuf();
        contents = buf.str();
    }
    

    Tokenizer tk(contents, contents.size());
    auto tokens = tk.tokenize();

    Parser p(std::move(tokens));
    auto prog = p.parseProg();

    if (!prog.has_value()) {
        std::cerr << "Invalid program" << std::endl;
        exit(1);
    }

    Generator g(prog.value());

    {
        std::ofstream file("out.asm");
        file << g.genProg();
    }

    system("nasm -felf64 out.asm");
    system("ld out.o -o out");

    return 0;
}

