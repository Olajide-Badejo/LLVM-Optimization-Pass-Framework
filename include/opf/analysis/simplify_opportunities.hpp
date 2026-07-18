//===- simplify_opportunities.hpp - count simplifiable patterns ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// A new PassManager function analysis that counts, without changing anything,
// the simplification opportunities the transform passes will later act on:
// constant foldable binary ops, algebraic identities, strength reducible
// multiplies, and duplicate pure computations within a block. Because it shares
// its predicates with the transforms, the analysis doubles as a check that the
// transforms found everything they should have. The predicates are free
// functions for direct unit testing.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_ANALYSIS_SIMPLIFY_OPPORTUNITIES_HPP
#define OPF_ANALYSIS_SIMPLIFY_OPPORTUNITIES_HPP

#include "llvm/IR/PassManager.h"
#include "llvm/Support/JSON.h"

namespace llvm {
class Function;
class Instruction;
class raw_ostream;
} // namespace llvm

namespace opf {

// A binary operator whose operands are both integer constants, so it folds to a
// single constant.
bool isConstantFoldable(const llvm::Instruction &Inst);

// A binary operator that equals one of its inputs or a constant by an algebraic
// identity: add x 0, sub x x, mul x 1, mul x 0, and x -1, or x 0, xor x 0, and
// the shift by zero and self and/or/xor cases.
bool isAlgebraicIdentity(const llvm::Instruction &Inst);

// A multiply by a constant power of two greater than one, which a shift
// replaces. The by one case is an identity, not a strength reduction, so it is
// excluded here.
bool isStrengthReducibleMul(const llvm::Instruction &Inst);

// Number of pure computations in the function that repeat an earlier identical
// computation within the same block. Commutative operands are ordered before
// comparison so add a b and add b a count as duplicates.
unsigned countDuplicatePureComputations(const llvm::Function &F);

struct SimplifyOpportunities {
  unsigned NumConstantFoldable = 0;
  unsigned NumAlgebraicIdentities = 0;
  unsigned NumStrengthReducible = 0;
  unsigned NumDuplicatePureComputations = 0;

  unsigned total() const {
    return NumConstantFoldable + NumAlgebraicIdentities + NumStrengthReducible +
           NumDuplicatePureComputations;
  }

  llvm::json::Value toJSON() const;
  void print(llvm::raw_ostream &OS) const;
};

class SimplifyOpportunitiesAnalysis
    : public llvm::AnalysisInfoMixin<SimplifyOpportunitiesAnalysis> {
  friend llvm::AnalysisInfoMixin<SimplifyOpportunitiesAnalysis>;
  static llvm::AnalysisKey Key;

public:
  using Result = SimplifyOpportunities;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

// Registered under the pipeline name "my-print-simplify-opps".
class SimplifyOpportunitiesPrinter
    : public llvm::PassInfoMixin<SimplifyOpportunitiesPrinter> {
  llvm::raw_ostream &OS;

public:
  explicit SimplifyOpportunitiesPrinter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace opf

#endif // OPF_ANALYSIS_SIMPLIFY_OPPORTUNITIES_HPP
