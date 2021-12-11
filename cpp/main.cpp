#include <iostream>
#include <sstream>
#include <optional>

#include "lexer.cpp"
#include "filename.cpp"
#include "pos.cpp"

int main (int argc, char** argv) {
    if (argc == 2) {
        std::string input;
        if (strcmp(argv[1], "-") == 0) {
            std::stringstream ss;
            ss << std::cin.rdbuf();
            input = ss.str();
        } else {
            input = argv[1];
        }

        brmh::Lexer tokens(brmh::Filename::create("<CLI arg>"), input.c_str());
        for (auto tok = std::optional<brmh::Lexer::Token>(); (tok = tokens.next());) {
            tok.value().print(std::cout);
            std::cout << std::endl;
        }

        return EXIT_SUCCESS;
    } else {
        std::cerr << "Invalid CLI arguments" << std::endl;
        return EXIT_FAILURE;
    }
}
