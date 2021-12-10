#include <iostream>
#include <optional>

#include "lexer.cpp"
#include "filename.cpp"
#include "pos.cpp"

int main (int argc, char** argv) {
    if (argc > 1) {
        brmh::Lexer tokens(brmh::Filename::create("<CLI arg>"), argv[1]);

        while (true) {
            const auto c = tokens.next();
            if (c) {
                c.value().print(std::cout);
                std::cout << std::endl;
            } else {
                break;
            }
        }

        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
