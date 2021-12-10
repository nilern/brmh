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
                std::cout << *c.value().chars << ", ";
            } else {
                break;
            }
        }
        std::cout << std::endl;

        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
