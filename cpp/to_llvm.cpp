#include "to_llvm.hpp"

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "hossa.hpp"

namespace brmh {

llvm::Value* hossa::AddWI64::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    return builder.CreateAdd(l, r);
}

llvm::Value* hossa::SubWI64::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    return builder.CreateSub(l, r);
}

llvm::Value* hossa::MulWI64::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    return builder.CreateMul(l, r);
}

llvm::Value* hossa::Bool::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx.llvm_ctx), value);
}

llvm::Value* hossa::I64::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx.llvm_ctx), /* FIXME: do this in typechecking, with range checking: */ atol(digits));
}

llvm::Value* hossa::Param::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return ctx.exprs.at(this);
}

void hossa::If::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    llvm::Value* const llvm_cond = builder.CreateTrunc(cond->to_llvm(ctx, builder), llvm::Type::getInt1Ty(ctx.llvm_ctx));
    llvm::BasicBlock* const llvm_conseq = conseq->to_llvm(ctx, builder);
    llvm::BasicBlock* const llvm_alt = alt->to_llvm(ctx, builder);
    builder.CreateCondBr(llvm_cond, llvm_conseq, llvm_alt);
}

void hossa::Goto::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    llvm::BasicBlock* const llvm_dest = dest->to_llvm(ctx, builder);
    builder.CreateBr(llvm_dest);
}

void hossa::Return::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    builder.CreateRet(res->to_llvm(ctx, builder));
}

// FIXME: Block arguments -> Phi functions
llvm::BasicBlock* hossa::Block::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto const it = ctx.blocks.find(this);
    if (it != ctx.blocks.end()) {
        return it->second;
    } else {
        llvm::BasicBlock* const llvm_block = llvm::BasicBlock::Create(ctx.llvm_ctx, name.src_name(ctx.names).unwrap_or(""), ctx.fn);
        ctx.blocks.insert({this, llvm_block});

        builder.SetInsertPoint(llvm_block);
        transfer->to_llvm(ctx, builder);

        return llvm_block;
    }
}

void hossa::Fn::to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const {
    std::span<Param*> params = entry->params;
    std::vector<llvm::Type*> llvm_domain(params.size());
    std::transform(params.begin(), params.end(), llvm_domain.begin(), [&] (Param* param) {
        return param->type->to_llvm(llvm_ctx);
    });
    llvm::FunctionType* const llvm_type = llvm::FunctionType::get(codomain->to_llvm(llvm_ctx), llvm_domain, false);

    llvm::Twine llvm_name(name.src_name(names).unwrap_or(""));
    llvm::Function* const llvm_fn = llvm::Function::Create(llvm_type, linkage, llvm_name, module);

    ToLLVMCtx ctx(names, llvm_ctx, llvm_fn);
    llvm::IRBuilder builder(llvm_ctx);

    std::size_t i = 0;
    for (auto& arg : llvm_fn->args()) {
        Param* const param = params[i++];
        arg.setName(llvm::Twine(param->name.src_name(names).unwrap_or("")));
        ctx.exprs.insert({param, &arg});
    }

    entry->to_llvm(ctx, builder);

    llvm::verifyFunction(*llvm_fn);
}

void hossa::Program::to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const {
    for (hossa::Fn* const ext_fn : externs) {
        ext_fn->to_llvm(names, llvm_ctx, module, llvm::Function::ExternalLinkage);
    }
}

} // namespace brmh
