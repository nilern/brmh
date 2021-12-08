# brmh

## Language

* ML-ish systems programming
* Transparent or zero-cost language features
* Module system (second-class)
    - Recursive modules
    - Applicative and generative functors
        * Safe applicativity (F-ing Modules)
    - Implicit functors (instead of typeclasses)
        * Can use F-ing modules identity tracking phantom types for coherence
    - Monomorphic modules can be used as first-class?
        * Mostly for runtime polymorphism (trait objects / interface types)
* HM/bidirectional type inference
* Monomorphize polymorphic functions and functors
    - Unify them via zero-size type witnesses?
    - Cannot monomorphise recursive uses of generative functors
* Raw and hygienic macros
* GC, but bypassable (like Golang)
* Context managers (like Python, though `with` can be just another macro)
    - Instead of destructors/`defer`
    - For non-memory resources (e.g. files, locks)

## Implementation

* In (simple) C++ for portability and community
* Unity build
