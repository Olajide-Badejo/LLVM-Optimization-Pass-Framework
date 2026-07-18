//===- dce.hpp - dead code elimination -----------------------------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// Removes trivially dead instructions and, through a worklist, the instructions
// that only fed them. It uses the same isTriviallyDead predicate as the dead
// code analysis, so what the analysis reports is exactly what this removes.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_TRANSFORMS_DCE_HPP
#define OPF_TRANSFORMS_DCE_HPP

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
} // namespace llvm

namespace opf {

// Erases trivially dead instructions to a fixed point. Returns the number
// removed.
unsigned dceFunction(llvm::Function &F);

// Registered under the pipeline name "my-dce".
class DCEPass : public llvm::PassInfoMixin<DCEPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

} // namespace opf

#endif // OPF_TRANSFORMS_DCE_HPP
