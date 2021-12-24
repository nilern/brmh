#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "hossa.hpp"

namespace brmh {

llvm::Value* hossa::Int::to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*>, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_ctx), /* FIXME: do this in typechecking, with range checking: */ atol(digits));
}

llvm::Value* hossa::Param::to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*> env, llvm::LLVMContext&, llvm::IRBuilder<>&) const {
    return env.at(this);
}

void hossa::Return::to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*> env, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder) const {
    builder.CreateRet(res->to_llvm(env, llvm_ctx, builder));
}

void hossa::Fn::to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const {
    std::unordered_map<const hossa::Param*, llvm::Value*> env;
    llvm::IRBuilder builder(llvm_ctx);

    llvm::Twine llvm_name(name.src_name(names).unwrap_or(""));

    std::span<Param*> params = entry->params;
    std::vector<llvm::Type*> llvm_domain(params.size());
    std::transform(params.begin(), params.end(), llvm_domain.begin(), [&] (Param* param) {
        return param->type->to_llvm(llvm_ctx);
    });
    llvm::FunctionType* const llvm_type = llvm::FunctionType::get(codomain->to_llvm(llvm_ctx), llvm_domain, false);

    llvm::Function* const llvm_fn = llvm::Function::Create(llvm_type, linkage, llvm_name, module);

    std::size_t i = 0;
    for (auto& arg : llvm_fn->args()) {
        Param* const param = params[i++];
        arg.setName(llvm::Twine(param->name.src_name(names).unwrap_or("")));
        env.insert({param, &arg});
    }

    llvm::BasicBlock* llvm_entry = llvm::BasicBlock::Create(llvm_ctx, entry->name.src_name(names).unwrap_or(""), llvm_fn);
    builder.SetInsertPoint(llvm_entry);
    entry->transfer->to_llvm(env, llvm_ctx, builder);

    llvm::verifyFunction(*llvm_fn);
}

void hossa::Program::to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const {
    for (hossa::Fn* const ext_fn : externs) {
        ext_fn->to_llvm(names, llvm_ctx, module, llvm::Function::ExternalLinkage);
    }
}

} // namespace brmh
