#include "to_llvm.hpp"

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "hossa.hpp"
#include "hossa_doms.hpp"

namespace brmh {

llvm::Value* hossa::Call::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    llvm::Value* const llvm_callee = callee()->to_llvm(ctx, builder);

    std::vector<llvm::Value*> llvm_args(args().size());
    std::transform(args().begin(), args().end(), llvm_args.begin(), [&] (hossa::Expr* arg) {
        return arg->to_llvm(ctx, builder);
    });

    return builder.CreateCall(static_cast<llvm::Function*>(llvm_callee), llvm_args);
}

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

llvm::Value* hossa::EqI64::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    llvm::Value* const bit = builder.CreateICmpEQ(l, r);
    return builder.CreateZExt(bit, type->to_llvm(ctx.llvm_ctx));
}

llvm::Value* hossa::Bool::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(type->to_llvm(ctx.llvm_ctx), value);
}

llvm::Value* hossa::I64::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(type->to_llvm(ctx.llvm_ctx), /* FIXME: do this in typechecking, with range checking: */ atol(digits));
}

llvm::Value* hossa::Param::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return ctx.exprs.at(this);
}

void hossa::If::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    llvm::Value* const bit_cond = cond->to_llvm(ctx, builder);
    llvm::Value* const llvm_cond = builder.CreateTrunc(bit_cond, llvm::Type::getInt1Ty(ctx.llvm_ctx));
    llvm::BasicBlock* const llvm_block = builder.GetInsertBlock();
    llvm::BasicBlock* const llvm_conseq = ctx.blocks.at(conseq);
    llvm::BasicBlock* const llvm_alt = ctx.blocks.at(alt);
    builder.SetInsertPoint(llvm_block);
    builder.CreateCondBr(llvm_cond, llvm_conseq, llvm_alt);
}

// TODO: Ensure TCO:
void hossa::TailCall::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    llvm::Value* const llvm_callee = callee()->to_llvm(ctx, builder);

    std::vector<llvm::Value*> llvm_args(args().size());
    std::transform(args().begin(), args().end(), llvm_args.begin(), [&] (hossa::Expr* arg) {
        return arg->to_llvm(ctx, builder);
    });

    llvm::Value* res = builder.CreateCall(static_cast<llvm::Function*>(llvm_callee), llvm_args);
    builder.CreateRet(res);
}

void hossa::Goto::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    llvm::BasicBlock* const llvm_dest = ctx.blocks.at(dest);
    builder.CreateBr(llvm_dest);
}

void hossa::Return::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    builder.CreateRet(res->to_llvm(ctx, builder));
}

void hossa::Block::llvm_declare(ToLLVMCtx& ctx) const {
    llvm::BasicBlock* const llvm_block = llvm::BasicBlock::Create(ctx.llvm_ctx, name.src_name(ctx.names).unwrap_or(""), ctx.fn);
    ctx.blocks.insert({this, llvm_block});
}

// FIXME: Block arguments -> Phi functions
llvm::BasicBlock* hossa::Block::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    llvm::BasicBlock* const llvm_block = ctx.blocks.at(this);

    builder.SetInsertPoint(llvm_block);
    transfer->to_llvm(ctx, builder);

    return llvm_block;
}

void hossa::Fn::llvm_declare(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const {
    type::FnType* const hossa_type = static_cast<type::FnType*>(type); // HACK: static_cast
    std::vector<llvm::Type*> llvm_domain(hossa_type->domain.size());
    std::transform(hossa_type->domain.begin(), hossa_type->domain.end(), llvm_domain.begin(), [&] (type::Type* param_type) {
        return param_type->to_llvm(llvm_ctx);
    });
    llvm::FunctionType* const llvm_type = llvm::FunctionType::get(hossa_type->codomain->to_llvm(llvm_ctx), llvm_domain, false);

    llvm::Twine llvm_name(name.src_name(names).unwrap_or(""));
    llvm::Function* llvm_fn = llvm::Function::Create(llvm_type, linkage, llvm_name, module);

    std::size_t i = 0;
    for (auto& arg : llvm_fn->args()) {
        Param* const param = entry->params[i++];
        arg.setName(llvm::Twine(param->name.src_name(names).unwrap_or("")));
    }
}

void hossa::Fn::llvm_define(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const {
    llvm::Function* const llvm_fn = module.getFunction(name.src_name(names).unwrap_or(""));
    ToLLVMCtx ctx(names, llvm_ctx, module, llvm_fn);
    llvm::IRBuilder builder(llvm_ctx);

    // Push params to `ctx`:
    std::size_t i = 0;
    for (auto& arg : llvm_fn->args()) {
        Param* const param = entry->params[i++];
        ctx.exprs.insert({param, &arg});
    }

    // Create empty blocks:
    hossa::doms::DomTree const doms = hossa::doms::DomTree::of(this);
    doms.pre_visit_blocks([&] (hossa::Block const* block) {
        block->llvm_declare(ctx);
    });

    // Fill blocks in dominator tree preorder:
    doms.pre_visit_blocks([&] (hossa::Block const* block) {
        block->to_llvm(ctx, builder);
    });

    llvm::verifyFunction(*llvm_fn);
}

llvm::Value* hossa::Fn::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &) const {
    return ctx.llvm_module.getFunction(name.src_name(ctx.names).unwrap_or(""));
}

void hossa::Program::to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const {
    for (hossa::Fn* const ext_fn : externs) {
        ext_fn->llvm_declare(names, llvm_ctx, module, llvm::Function::ExternalLinkage);
    }

    for (hossa::Fn* const ext_fn : externs) {
        ext_fn->llvm_define(names, llvm_ctx, module);
    }
}

} // namespace brmh
