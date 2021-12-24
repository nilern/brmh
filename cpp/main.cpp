#include <iostream>
#include <sstream>
#include <optional>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include "util.cpp"
#include "bumparena.cpp"
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

#include "fast.cpp"

#include "typeenv.cpp"
#include "typer.cpp"

#include "hossa.cpp"

#include "to_hossa.cpp"

#include "to_llvm.cpp"

// TODO: Memory management (using bumpalo arenas and taking advantage of "IR going through passes" nature)

int main (int argc, char** argv) {
    if (argc == 2) {
        const brmh::Src src = strcmp(argv[1], "-") == 0
                ? brmh::Src::stdin()
                : brmh::Src::cli_arg(argv[1]);

        try {
            std::cout << "Tokens\n======" << std::endl << std::endl;

            brmh::Lexer tokens(src);
            for (std::optional<brmh::Lexer::Token> tok; (tok = tokens.next());) {
                tok.value().print(std::cout);
                std::cout << std::endl;
            }

            std::cout << std::endl << "AST\n===" << std::endl << std::endl;

            brmh::Names names;
            brmh::type::Types types;

            brmh::Parser parser(brmh::Lexer(src), names, types);
            brmh::ast::Program program = parser.program();
            program.print(names, std::cout);

            std::cout << "F-AST\n=====" << std::endl << std::endl;

            brmh::fast::Program typed_program = program.check(types);
            typed_program.print(names, std::cout);

            std::cout << "HO-SSA\n======" << std::endl << std::endl;

            brmh::hossa::Program hossa_program = typed_program.to_hossa(names);
            hossa_program.print(names, std::cout);

            std::cout << "LLVM IR\n=======" << std::endl << std::endl;

            llvm::InitializeAllTargetInfos();
            llvm::InitializeAllTargets();
            llvm::InitializeAllTargetMCs();
            llvm::InitializeAllAsmParsers();
            llvm::InitializeAllAsmPrinters();

            llvm::LLVMContext llvm_ctx;

            auto target_triple = llvm::sys::getDefaultTargetTriple();
            std::string triple_error;
            auto target = llvm::TargetRegistry::lookupTarget(target_triple, triple_error);
            if (!target) {
              llvm::errs() << triple_error;
              return EXIT_FAILURE;
            }

            auto cpu = "generic";
            auto features = "";
            llvm::TargetOptions opt;
            auto reloc_model = llvm::Optional<llvm::Reloc::Model>();
            auto target_machine = target->createTargetMachine(target_triple, cpu, features, opt, reloc_model);

            llvm::Module llvm_module("bmrh program", llvm_ctx);
            llvm_module.setTargetTriple(target_triple);
            llvm_module.setDataLayout(target_machine->createDataLayout());
            hossa_program.to_llvm(names, llvm_ctx, llvm_module);

            for (const auto& fn : llvm_module.functions()) {
                fn.print(llvm::errs());
                std::cout << std::endl;
            }

            std::cout << ">>> Generating object file..." << std::endl << std::endl;

            auto filename = "output.o";
            std::error_code outfile_error;
            llvm::raw_fd_ostream outfile(filename, outfile_error, llvm::sys::fs::OF_None);
            if (outfile_error) {
              llvm::errs() << "Could not open file: " << outfile_error.message();
              return EXIT_FAILURE;
            }

            llvm::legacy::PassManager pass;
            auto FileType = llvm::CGFT_ObjectFile;

            if (target_machine->addPassesToEmitFile(pass, outfile, nullptr, FileType)) {
              llvm::errs() << "TheTargetMachine can't emit a file of this type";
              return 1;
            }

            pass.run(llvm_module);
            outfile.flush();

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
