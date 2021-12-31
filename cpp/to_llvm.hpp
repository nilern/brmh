#ifndef TO_LLVM_HPP
#define TO_LLVM_HPP

#include <unordered_map>

#include "llvm/IR/LLVMContext.h"

#include "hossa.hpp"

namespace brmh {

struct ToLLVMCtx {
    llvm::LLVMContext& llvm_ctx;
    std::unordered_map<const hossa::Expr*, llvm::Value*> exprs;

    ToLLVMCtx(llvm::LLVMContext& llvm_ctx_)
        : llvm_ctx(llvm_ctx_), exprs() {}
};

} // namespace brmh

#endif // TO_LLVM_HPP
