#include <iostream>
#include <sstream>
#include <optional>

#include "filename.cpp"
#include "pos.cpp"
#include "src.cpp"
#include "span.cpp"
#include "name.cpp"

#include "ast.cpp"

#include "lexer.cpp"
#include "parser.cpp"

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

        std::cout << std::endl;

        brmh::Names names;

        brmh::Parser parser(brmh::Lexer(src), names);
        std::optional<brmh::ast::Node*> expr = parser.expr();
        if (!expr) {
            std::cerr << "Parse error" << std::endl;
            return EXIT_FAILURE;
        }
        (*expr)->print(std::cout);
        std::cout << std::endl;

        delete *expr;

        return EXIT_SUCCESS;
    } else {
        std::cerr << "Invalid CLI arguments" << std::endl;
        return EXIT_FAILURE;
    }
}
