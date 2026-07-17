//===- dead_code_report.hpp - dead instructions and chain depth ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//
//
// A new PassManager function analysis over the use graph. It reports how many
// instructions are trivially dead (removable without changing behavior) and the
// depth of the longest use-def chain, a proxy for how much instruction level
// parallelism the straight line code exposes. The predicates are free functions
// so the unit tests can assert on them directly against hand built IR.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_ANALYSIS_DEAD_CODE_REPORT_HPP
#define OPF_ANALYSIS_DEAD_CODE_REPORT_HPP

#include "llvm/IR/PassManager.h"
#include "llvm/Support/JSON.h"

namespace llvm {
class Function;
class Instruction;
class raw_ostream;
} // namespace llvm

namespace opf {

// True when Inst can be deleted without changing observable behavior: it feeds
// no other instruction, ends no block, and has no side effects. This is the
// same conservative notion the DCE transform relies on, kept in one place so
// the analysis and the transform never drift apart.
bool isTriviallyDead(const llvm::Instruction &Inst);

// Depth of the longest chain of data dependencies ending at Inst. A value with
// no instruction operands has depth 1. Phi nodes are treated as chain roots
// (depth 1) so a loop carried dependency does not create an unbounded chain.
struct ChainDepth {
  unsigned Max = 0;
  double Mean = 0.0;
};
ChainDepth useDefChainDepth(const llvm::Function &F);

struct DeadCodeReport {
  unsigned NumInstructions = 0;
  unsigned NumDeadInstructions = 0;
  unsigned MaxUseDefChainDepth = 0;
  double MeanUseDefChainDepth = 0.0;

  llvm::json::Value toJSON() const;
  void print(llvm::raw_ostream &OS) const;
};

class DeadCodeAnalysis : public llvm::AnalysisInfoMixin<DeadCodeAnalysis> {
  friend llvm::AnalysisInfoMixin<DeadCodeAnalysis>;
  static llvm::AnalysisKey Key;

public:
  using Result = DeadCodeReport;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

// Registered under the pipeline name "my-print-dead-code".
class DeadCodeReportPrinter
    : public llvm::PassInfoMixin<DeadCodeReportPrinter> {
  llvm::raw_ostream &OS;

public:
  explicit DeadCodeReportPrinter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace opf

#endif // OPF_ANALYSIS_DEAD_CODE_REPORT_HPP
