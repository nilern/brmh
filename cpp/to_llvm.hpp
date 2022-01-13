#ifndef TO_LLVM_HPP
#define TO_LLVM_HPP

#include <unordered_map>

#include "llvm/IR/LLVMContext.h"

#include "cps/cps.hpp"

namespace brmh {

struct ToLLVMCtx {
    Names const& names;
    llvm::LLVMContext& llvm_ctx;
    llvm::Module& llvm_module;
    llvm::Function* fn;
    std::unordered_map<cps::Block const*, std::vector<cps::Expr const*>> block_exprs;
    std::unordered_map<cps::Block const*, std::vector<cps::Block const*>> predecessors;
    std::unordered_map<cps::Block const*, std::vector<llvm::Value*>> successors_phi_inputs;
    std::unordered_map<const cps::Expr*, llvm::Value*> exprs;
    std::unordered_map<const cps::Block*, llvm::BasicBlock*> blocks;

    ToLLVMCtx(Names const& names_, llvm::LLVMContext& llvm_ctx_, llvm::Module& llvm_module_, llvm::Function* fn_,
              std::unordered_map<cps::Block const*, std::vector<cps::Expr const*>>&& block_exprs_,
              std::unordered_map<cps::Block const*, std::vector<cps::Block const*>>&& predecessors_)
        : names(names_), llvm_ctx(llvm_ctx_), llvm_module(llvm_module_), fn(fn_),
          block_exprs(block_exprs_), predecessors(predecessors_),
          successors_phi_inputs(), exprs(), blocks() {}
};

} // namespace brmh

#endif // TO_LLVM_HPP
