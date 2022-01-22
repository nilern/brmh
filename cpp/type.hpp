#ifndef BRMH_TYPE_HPP
#define BRMH_TYPE_HPP

#include <vector>
#include <ostream>

#include "llvm/IR/Type.h"

#include "span.hpp"
#include "name.hpp"
#include "error.hpp"

namespace brmh::type {

struct Uv;
struct FnType;
struct Bool;
struct I64;
class Types;

class Error : public BrmhError {
public:
    explicit Error(Span span);

    virtual const char* what() const noexcept override;

    Span span;
};

struct Type {
    virtual Type* find() { return this; }

    void occurs_check(Uv* uv, Span span) const;
    virtual void occurs_check_children(Uv* uv, Span span) const;

    void unify(Type* other, Span span);
    virtual void unifyFounds(Type* other, Span span) = 0;
    virtual void unifyFoundUvs(Uv* other, Span span);
    virtual void unifyFoundFns(FnType* other, Span span);
    virtual void unifyFoundBools(Bool* other, Span span);
    virtual void unifyFoundI64s(I64* other, Span span);

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) = 0;
};

struct Uv :public Type {
private:
    friend class Types;

    Name name_;
    opt_ptr<Type> parent_;
    std::size_t rank_;

    Uv(Name name)
        : Type(), name_(name), parent_(opt_ptr<Type>::none()), rank_(0) {}

public:
    virtual Type* find() override {
        return parent_.match<Type*>([&] (Type* parent) {
            Type* res = parent->find();
            parent_ = opt_ptr<Type>::some(res);
            return res;
        }, [&] () {
            return this;
        });
    }

    void union_(Uv* other) {
        assert(parent_.is_none());
        assert(other->parent_.is_none());
        assert(this != other);

        if (rank_ < other->rank_) {
            parent_ = opt_ptr<Type>::some(other);
        } else if (rank_ > other->rank_) {
            other->parent_ = opt_ptr<Type>::some(this);
        } else {
            parent_ = opt_ptr<Type>::some(other);
            other->rank_ += 1;
        }
    }

    void set(Type* other) {
        assert(!dynamic_cast<Uv*>(other));

        parent_ = opt_ptr<Type>::some(other);
    }

    virtual void occurs_check_children(Uv* uv, Span span) const override;

    virtual void unifyFounds(Type* other, Span span) override { other->unifyFoundUvs(this, span); }
    virtual void unifyFoundUvs(Uv* other, Span span) override;
    virtual void unifyFoundFns(FnType* other, Span span) override;
    virtual void unifyFoundBools(Bool* other, Span span) override;
    virtual void unifyFoundI64s(I64* other, Span span) override;

    virtual void print(Names const& names, std::ostream& dest) const override {
        parent_.match<void>([&] (Type const* parent) {
            parent->print(names, dest);
        }, [&] () {
            dest << '^';
            name_.print(names, dest);
        });
    }

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) override;
};

struct FnType : public Type {
    virtual void occurs_check_children(Uv* uv, Span span) const override;

    virtual void unifyFounds(Type* other, Span span) override { other->unifyFoundFns(this, span); }
    virtual void unifyFoundFns(FnType* other, Span span) override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) override;

    std::vector<Type*> domain;
    Type* codomain;

private:
    friend class Types;

    FnType(std::vector<Type*>&& domain, Type* codomain);
};

struct Bool : public Type {
    virtual void unifyFounds(Type* other, Span span) override { other->unifyFoundBools(this, span); }
    virtual void unifyFoundBools(Bool* other, Span span) override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) override;

private:
    friend class Types;

    Bool();
};

struct I64 : public Type {
    virtual void unifyFounds(Type* other, Span span) override { other->unifyFoundI64s(this, span); }
    virtual void unifyFoundI64s(I64* other, Span span) override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) override;

private:
    friend class Types;

    I64();
};

class Types {
    Names& names_;
    Bool* bool_;
    I64* i64_;

public:
    Types(Names& names) : names_(names), bool_(new Bool()), i64_(new I64()) {}

    Uv* uv() { return new Uv(names_.fresh()); }
    FnType* fn(std::vector<Type*>&& domain, Type* codomain);
    Bool* get_bool();
    I64* get_i64();
};

} // namespace brmh

#endif // BRMH_TYPE_HPP
