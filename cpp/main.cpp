#include <iostream>
#include <sstream>
#include <optional>

#include "util.cpp"
#include "filename.cpp"
#include "pos.cpp"
#include "src.cpp"
#include "span.cpp"
#include "name.cpp"
#include "error.cpp"

#include "type.cpp"

#include "ast.cpp"

#include "lexer.cpp"
#include "parser.cpp"

#include "typeenv.cpp"
#include "typer.cpp"

// TODO: Memory management (using bumpalo arenas and taking advantage of "IR going through passes" nature)

int main (int argc, char** argv) {
    if (argc == 2) {
        const brmh::Src src = strcmp(argv[1], "-") == 0
                ? brmh::Src::stdin()
                : brmh::Src::cli_arg(argv[1]);

        try {

            brmh::Lexer tokens(src);
            for (std::optional<brmh::Lexer::Token> tok; (tok = tokens.next());) {
                tok.value().print(std::cout);
                std::cout << std::endl;
            }

            std::cout << std::endl << std::endl;

            brmh::Names names;
            brmh::type::Types types;

            brmh::Parser parser(brmh::Lexer(src), names, types);
            brmh::ast::Program program = parser.program();
            program.print(names, std::cout);

            brmh::ast::Program typed_program = program.check(types);
            typed_program.print(names, std::cout);

            return EXIT_SUCCESS;
        } catch (const brmh::Lexer::Error& error) {
            std::cerr << error.what() << " at ";
            error.pos.print(std::cerr);
            return EXIT_FAILURE;
        } catch (const brmh::Parser::Error& error) {
            std::cerr << error.what() << " at ";
            error.pos.print(std::cerr);
            return EXIT_FAILURE;
        } catch (const brmh::BrmhError& error) {
            std::cerr << error.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "Invalid CLI arguments" << std::endl;
        return EXIT_FAILURE;
    }
}
