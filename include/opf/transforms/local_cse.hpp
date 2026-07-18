//===- local_cse.hpp - block local common subexpression cleanup ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// Within a single basic block, replaces a pure computation that repeats an
// earlier identical one with a reference to the first. It relies on
// canonicalization having already normalized commutative operand order, so add
// a b and add b a are seen as the same computation. Loads are never touched.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_TRANSFORMS_LOCAL_CSE_HPP
#define OPF_TRANSFORMS_LOCAL_CSE_HPP

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
} // namespace llvm

namespace opf {

// Removes block local duplicate pure computations. Returns the number removed.
unsigned localCSEFunction(llvm::Function &F);

// Registered under the pipeline name "my-local-cse".
class LocalCSEPass : public llvm::PassInfoMixin<LocalCSEPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

} // namespace opf

#endif // OPF_TRANSFORMS_LOCAL_CSE_HPP
