#include <iostream>
#include <sstream>
#include <optional>

#include "lexer.cpp"
#include "filename.cpp"
#include "pos.cpp"
#include "src.cpp"
#include "span.cpp"
#include "name.cpp"

#include "ast.hpp"

int main (int argc, char** argv) {
    if (argc == 2) {
        const brmh::Src src = strcmp(argv[1], "-") == 0
                ? brmh::Src::stdin()
                : brmh::Src::cli_arg(argv[1]);

        brmh::Lexer tokens(src);
        for (std::optional<brmh::Lexer::Token> tok; (tok = tokens.next());) {
            tok.value().print(std::cout);
            std::cout << std::endl;
        }

        return EXIT_SUCCESS;
    } else {
        std::cerr << "Invalid CLI arguments" << std::endl;
        return EXIT_FAILURE;
    }
}
