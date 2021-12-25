# brmh

## Language

* ML-ish systems programming
* Transparent or zero-cost language features
* Module system (second-class)
    - Recursive modules
    - Applicative and generative functors
        * Safe applicativity (F-ing Modules)
    - Higher-order functors
    - Implicit functors (instead of typeclasses)
        * Can use F-ing modules identity tracking phantom types for coherence
    - Monomorphic modules can be used as first-class?
        * Mostly for runtime polymorphism (trait objects / interface types)
* HM/bidirectional type inference
* Closures
* Monomorphize polymorphic functions and functors
    - Unify them via zero-size type witnesses?
    - Cannot monomorphise recursive uses of generative functors
* Raw and hygienic macros
* GC, but bypassable (like Golang)
* Move semantics for "non-POD" types
    - But no borrow checker; GC deals with (non-raw) pointer lifetimes
* Effect system
    - Effect interpreters use escape continuations (i.e. `longjmp`)
        * Region typing (MLKit, Rust) can make these continuations typesafe
* Context managers (like Python, though `with` can be just another macro)
    - Instead of destructors/`defer`
    - For non-memory resources (e.g. files, locks)

## Implementation

* In (simple) C++ for portability and community
* Unity build

### Dependencies

* `llvm`
