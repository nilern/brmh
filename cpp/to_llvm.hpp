#ifndef TO_LLVM_HPP
#define TO_LLVM_HPP

#include "llvm/IR/LLVMContext.h"

namespace brmh {

class ToLLVMCtx {
    llvm::LLVMContext& llvm_ctx;

public:
    ToLLVMCtx(llvm::LLVMContext& llvm_ctx);
};

} // namespace brmh

#endif // TO_LLVM_HPP
