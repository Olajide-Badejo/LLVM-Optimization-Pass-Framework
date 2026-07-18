//===- canonicalize.hpp - normalize commutative operand order ------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//
//
// Canonicalization does not remove anything; it reorders the operands of
// commutative instructions into a stable form (constants last, other operands
// by definition order). This exposes duplicates for local CSE and puts
// constants where the algebraic simplifier looks for them. The core is a free
// function returning the number of instructions it rewrote, so the fixed point
// pipeline can drive it and know whether it made progress.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_TRANSFORMS_CANONICALIZE_HPP
#define OPF_TRANSFORMS_CANONICALIZE_HPP

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
} // namespace llvm

namespace opf {

// Reorders commutative operands in place. Returns how many instructions were
// changed.
unsigned canonicalizeFunction(llvm::Function &F);

// Registered under the pipeline name "my-canon".
class CanonicalizePass : public llvm::PassInfoMixin<CanonicalizePass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

} // namespace opf

#endif // OPF_TRANSFORMS_CANONICALIZE_HPP
