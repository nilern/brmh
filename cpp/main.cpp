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

namespace brmh {

struct CLIArgs {
    std::string outfile;
    std::vector<std::string> infiles;

    class Error : public std::exception {
        virtual const char* what() const noexcept override { return "CLIArgs::Parser::Error"; }
    };

    static CLIArgs parse(std::size_t argc, char const* const* argv) {
        std::optional<std::string> outfile;
        std::vector<std::string> infiles;

        for (std::size_t i = 1 /* skip program name */; i < argc; ++i) {
            if (argv[i][0] == '-') {
                switch (argv[i][1]) {
                case 'o':
                    if (argv[i][2] == '\0') {
                        ++i;
                        if (i < argc) {
                            if (argv[i][0] != '-') {
                                outfile = argv[i];
                            } else {
                                throw Error(); // Missing outfile name
                            }
                        } else {
                            throw Error(); // Missing outfile name
                        }
                    } else {
                        throw Error(); // Too long option
                    }
                    break;
                default: throw Error(); // Unrecognized option
                }
            } else {
                infiles.push_back(argv[i]);
            }
        }

        return {.outfile = std::move(outfile.value_or("output.o")), .infiles = std::move(infiles)};
    }
};

} // namespace brmh

// TODO: Memory management (using bumpalo arenas and taking advantage of "IR going through passes" nature)

int main (int argc, char const* const* argv) {
    brmh::CLIArgs const args = brmh::CLIArgs::parse(argc, argv);

    if (args.infiles.size() == 0) {
        std::cerr << "No input files" << std::endl;
        return EXIT_FAILURE;
    } else if (args.infiles.size() > 1) {
        std::cerr << "TODO: multiple input files" << std::endl;
        return EXIT_FAILURE;
    } else {
        brmh::Src const src = brmh::Src::file(args.infiles[0].c_str());

        try {
            std::cout << "Tokens\n======" << std::endl << std::endl;

            brmh::Lexer tokens(src);
            std::optional<brmh::Lexer::Token> tok;
            do {
                tok = tokens.peek();
                tokens.next();
                if (tok) {
                    tok.value().print(std::cout);
                    std::cout << std::endl;
                }
            } while (tok);

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

            auto obj_filename = args.outfile + ".o";
            std::error_code outfile_error;
            llvm::raw_fd_ostream outfile(obj_filename, outfile_error, llvm::sys::fs::OF_None);
            if (outfile_error) {
                llvm::errs() << "Could not open file: " << outfile_error.message();
                return EXIT_FAILURE;
            }

            llvm::legacy::PassManager pass;
            auto FileType = llvm::CGFT_ObjectFile;
            if (target_machine->addPassesToEmitFile(pass, outfile, nullptr, FileType)) {
                llvm::errs() << "TheTargetMachine can't emit a file of this type";
                return EXIT_FAILURE;
            }

            pass.run(llvm_module);
            outfile.flush();

            std::cout << ">>> Linking program binary..." << std::endl << std::endl;

            // FIXME: portability:
            std::string link = std::string("cc -o")
                    .append(args.outfile)
                    .append(" ").append(obj_filename);
            if (std::system(link.c_str()) != 0) {
                std::remove(obj_filename.c_str()); // TODO: Error handling
                std::cerr << "Linking failed" << std::endl;
                return EXIT_FAILURE;
            } else {
                std::remove(obj_filename.c_str()); // TODO: Error handling
                return EXIT_SUCCESS;
            }
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
    }
}
