//===- algebraic_simplify.hpp - fold identities and strength reduce ------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// Rewrites integer binary operators that equal one of their inputs or a
// constant (add x 0, sub x x, mul x 1, mul x 0, and x -1, or x 0, xor x 0, the
// shift by zero and self and/or/xor cases) and strength reduces a multiply by a
// constant power of two into a shift. Every rewrite is value preserving on
// scalar integers; floating point, vectors, volatile, and atomic operations are
// left untouched. The core is a free function returning the number of rewrites.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_TRANSFORMS_ALGEBRAIC_SIMPLIFY_HPP
#define OPF_TRANSFORMS_ALGEBRAIC_SIMPLIFY_HPP

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
} // namespace llvm

namespace opf {

// Applies the algebraic rewrites to a fixed point within the function. Returns
// the number of instructions rewritten.
unsigned algebraicSimplifyFunction(llvm::Function &F);

// Registered under the pipeline name "my-simplify".
class AlgebraicSimplifyPass
    : public llvm::PassInfoMixin<AlgebraicSimplifyPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

} // namespace opf

#endif // OPF_TRANSFORMS_ALGEBRAIC_SIMPLIFY_HPP
