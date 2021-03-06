#include "to_llvm.hpp"

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "cps/cps.hpp"
#include "cps/doms.hpp"
#include "cps/schedule.hpp"

namespace brmh {

llvm::Value* cps::Expr::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder) const {
    auto it = ctx.exprs.find(this);
    if (it != ctx.exprs.end()) {
        return it->second;
    } else {
        llvm::Value* const res = do_to_llvm(ctx, builder);
        ctx.exprs.insert({this, res});
        return res;
    }
}

llvm::Value* cps::AddWI64::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    return builder.CreateAdd(l, r);
}

llvm::Value* cps::SubWI64::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    return builder.CreateSub(l, r);
}

llvm::Value* cps::MulWI64::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    return builder.CreateMul(l, r);
}

llvm::Value* cps::EqI64::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    auto l = args[0]->to_llvm(ctx, builder);
    auto r = args[1]->to_llvm(ctx, builder);
    llvm::Value* const bit = builder.CreateICmpEQ(l, r);
    return builder.CreateZExt(bit, type->to_llvm(ctx.llvm_ctx));
}

llvm::Value* cps::Bool::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(type->to_llvm(ctx.llvm_ctx), value);
}

llvm::Value* cps::I64::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return llvm::ConstantInt::get(type->to_llvm(ctx.llvm_ctx), /* FIXME: do this in typechecking, with range checking: */ atol(digits));
}

llvm::Value* cps::Param::do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>&) const {
    return ctx.exprs.at(this);
}

void cps::If::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder, Block const* block) const {
    llvm::Value* const bit_cond = ctx.exprs.at(cond);
    llvm::Value* const llvm_cond = builder.CreateTrunc(bit_cond, llvm::Type::getInt1Ty(ctx.llvm_ctx));
    llvm::BasicBlock* const llvm_block = builder.GetInsertBlock();
    llvm::BasicBlock* const llvm_conseq = ctx.blocks.at(static_cast<cps::Block*>(conseq));
    llvm::BasicBlock* const llvm_alt = ctx.blocks.at(static_cast<cps::Block*>(alt));
    builder.SetInsertPoint(llvm_block);
    builder.CreateCondBr(llvm_cond, llvm_conseq, llvm_alt);

    ctx.successors_phi_inputs.insert({block, {}});
}

class GotoToLLVM : public cps::ContVisitor {
    ToLLVMCtx& ctx_;
    llvm::IRBuilder<>& builder_;
    cps::Block const* block_;
    llvm::Value* res_;

public:
    GotoToLLVM(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder, cps::Block const* block, llvm::Value* res)
        : ctx_(ctx), builder_(builder), block_(block), res_(res) {}

    virtual void visit(cps::Block const* dest) override {
        builder_.CreateBr(ctx_.blocks.at(dest));
        ctx_.successors_phi_inputs.insert({block_, {res_}});
    }

    virtual void visit(cps::Return const*) override {
        builder_.CreateRet(res_);
    }
};

// TODO: Ensure TCO:
void cps::Call::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder, Block const* block) const {
    llvm::Value* const llvm_callee = ctx.exprs.at(callee());

    std::vector<llvm::Value*> llvm_args(args().size());
    std::transform(args().begin(), args().end(), llvm_args.begin(), [&] (cps::Expr* arg) {
        return ctx.exprs.at(arg);
    });

    llvm::Value* res = builder.CreateCall(static_cast<llvm::Function*>(llvm_callee), llvm_args);

    GotoToLLVM visitor(ctx, builder, block, res);
    cont->accept(visitor);
}

void cps::Goto::to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &builder, Block const* block) const {
    GotoToLLVM visitor(ctx, builder, block, ctx.exprs.at(res));
    dest->accept(visitor);
}

void cps::Block::llvm_declare(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    llvm::BasicBlock* const llvm_block = llvm::BasicBlock::Create(ctx.llvm_ctx, name.src_name(ctx.names).unwrap_or(""), ctx.fn);
    ctx.blocks.insert({this, llvm_block});

    // Create Phis for params:
    auto it = ctx.predecessors.find(this);
    std::size_t const predecessor_count = (it != ctx.predecessors.end()) ? it->second.size() : 0;
    if (predecessor_count > 0) { // Except for entry (or unused block, but that can't occur)
        builder.SetInsertPoint(llvm_block);
        for (Param const* param : params) {
            llvm::PHINode* const phi = builder.CreatePHI(param->type->to_llvm(ctx.llvm_ctx), predecessor_count);
            ctx.exprs.insert({param, phi});
        }
    }
}

void cps::Block::to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const {
    llvm::BasicBlock* const llvm_block = ctx.blocks.at(this);
    builder.SetInsertPoint(llvm_block);

    for (Expr const* expr : ctx.block_exprs.at(this)) {
        expr->to_llvm(ctx, builder);
    }
    transfer->to_llvm(ctx, builder, this);
}

void cps::Block::llvm_patch_phis(ToLLVMCtx &ctx) const {
    llvm::BasicBlock* const llvm_block = ctx.blocks.at(this);
    std::size_t i = 0;
    for (llvm::PHINode& phi : llvm_block->phis()) {
        for (cps::Block const* predecessor : ctx.predecessors.at(this)) {
            llvm::Value* const arg = ctx.successors_phi_inputs.at(predecessor)[i];
            phi.addIncoming(arg, ctx.blocks.at(predecessor));
        }
        ++i;
    }
}

void cps::Fn::llvm_declare(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const {
    type::FnType* const cps_type = static_cast<type::FnType*>(type); // HACK: static_cast
    std::vector<llvm::Type*> llvm_domain(cps_type->domain.size());
    std::transform(cps_type->domain.begin(), cps_type->domain.end(), llvm_domain.begin(), [&] (type::Type* param_type) {
        return param_type->to_llvm(llvm_ctx);
    });
    llvm::FunctionType* const llvm_type = llvm::FunctionType::get(cps_type->codomain->to_llvm(llvm_ctx), llvm_domain, false);

    llvm::Twine llvm_name(name.src_name(names).unwrap_or(""));
    llvm::Function* llvm_fn = llvm::Function::Create(llvm_type, linkage, llvm_name, module);

    std::size_t i = 0;
    for (auto& arg : llvm_fn->args()) {
        Param* const param = entry->params[i++];
        arg.setName(llvm::Twine(param->name.src_name(names).unwrap_or("")));
    }
}

void cps::Fn::llvm_define(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const {
    cps::doms::DomTree const doms = cps::doms::DomTree::of(this);

    cps::schedule::Schedule const schedule = cps::schedule::schedule_late(this, doms);
    std::unordered_map<Block const*, std::vector<Expr const*>> block_exprs;
    for (auto expr_block : schedule) {
        Expr const* const expr = expr_block.first;
        Block const* const block = expr_block.second;
        auto it = block_exprs.find(block);
        if (it != block_exprs.end()) {
            it->second.push_back(expr);
        } else {
            block_exprs.insert({block, {expr}});
        }
    }

    std::unordered_map<Block const*, std::vector<Block const*>> predecessors;
    doms.pre_visit_blocks([&] (cps::Block const* block) {
        for (Cont const* succ : block->transfer->successors()) {
            succ->as_block().iter([&] (Block const* succ) {
                auto it = predecessors.find(succ);
                if (it != predecessors.end()) {
                    it->second.push_back(block);
                } else {
                    predecessors.insert({succ, {block}});
                }
            });
        }
    });

    llvm::Function* const llvm_fn = module.getFunction(name.src_name(names).unwrap_or(""));
    ToLLVMCtx ctx(names, llvm_ctx, module, llvm_fn, std::move(block_exprs), std::move(predecessors));
    llvm::IRBuilder builder(llvm_ctx);

    // Push params to `ctx`:
    std::size_t i = 0;
    for (auto& arg : llvm_fn->args()) {
        Param* const param = entry->params[i++];
        ctx.exprs.insert({param, &arg});
    }

    // Create empty blocks, in dominator tree preorder since LLVM wants the entry block first:
    doms.pre_visit_blocks([&] (cps::Block const* block) {
        block->llvm_declare(ctx, builder);
    });

    // Fill blocks, in dominator tree preorder so that shared Values are created in postorder:
    doms.pre_visit_blocks([&] (cps::Block const* block) {
        block->to_llvm(ctx, builder);
    });

    // Fill Phi sources:
    doms.pre_visit_blocks([&] (cps::Block const* block) {
        block->llvm_patch_phis(ctx);
    });

    llvm::verifyFunction(*llvm_fn);
}

llvm::Value* cps::Fn::do_to_llvm(ToLLVMCtx &ctx, llvm::IRBuilder<> &) const {
    return ctx.llvm_module.getFunction(name.src_name(ctx.names).unwrap_or(""));
}

void cps::Program::to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const {
    for (cps::Fn* const ext_fn : externs) {
        ext_fn->llvm_declare(names, llvm_ctx, module, llvm::Function::ExternalLinkage);
    }

    for (cps::Fn* const ext_fn : externs) {
        ext_fn->llvm_define(names, llvm_ctx, module);
    }
}

} // namespace brmh
