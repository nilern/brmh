#ifndef TO_LLVM_HPP
#define TO_LLVM_HPP

#include <unordered_map>

#include "llvm/IR/LLVMContext.h"

#include "hossa.hpp"

namespace brmh {

struct ToLLVMCtx {
    Names const& names;
    llvm::LLVMContext& llvm_ctx;
    llvm::Function* fn;
    std::unordered_map<const hossa::Expr*, llvm::Value*> exprs;
    std::unordered_map<const hossa::Block*, llvm::BasicBlock*> blocks;

    ToLLVMCtx(Names const& names_, llvm::LLVMContext& llvm_ctx_, llvm::Function* fn_)
        : names(names_), llvm_ctx(llvm_ctx_), fn(fn_), exprs(), blocks() {}
};

} // namespace brmh

#endif // TO_LLVM_HPP
