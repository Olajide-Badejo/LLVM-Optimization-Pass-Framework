//===- dead_code_report.cpp - dead instructions and chain depth ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/analysis/dead_code_report.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

using namespace llvm;

namespace opf {

bool isTriviallyDead(const Instruction &Inst) {
  if (!Inst.use_empty())
    return false;
  if (Inst.isTerminator())
    return false;
  // mayHaveSideEffects covers stores, volatile and atomic accesses, calls that
  // are not readnone/willreturn, and anything that can throw or not return.
  if (Inst.mayHaveSideEffects())
    return false;
  return true;
}

// Recursive longest chain with memoization. Recursion stops at phi nodes, which
// keeps the traversal acyclic even in the presence of loops.
static unsigned depthOf(const Instruction *Inst,
                        DenseMap<const Instruction *, unsigned> &Memo) {
  auto Found = Memo.find(Inst);
  if (Found != Memo.end())
    return Found->second;

  unsigned Best = 0;
  if (!isa<PHINode>(Inst)) {
    for (const Use &Op : Inst->operands())
      if (const auto *OpInst = dyn_cast<Instruction>(Op.get()))
        Best = std::max(Best, depthOf(OpInst, Memo));
  }

  unsigned Depth = Best + 1;
  Memo[Inst] = Depth;
  return Depth;
}

ChainDepth useDefChainDepth(const Function &F) {
  ChainDepth Result;
  if (F.isDeclaration())
    return Result;

  DenseMap<const Instruction *, unsigned> Memo;
  unsigned long Sum = 0;
  unsigned Count = 0;
  for (const BasicBlock &BB : F)
    for (const Instruction &Inst : BB) {
      unsigned Depth = depthOf(&Inst, Memo);
      Result.Max = std::max(Result.Max, Depth);
      Sum += Depth;
      ++Count;
    }

  if (Count)
    Result.Mean = static_cast<double>(Sum) / static_cast<double>(Count);
  return Result;
}

json::Value DeadCodeReport::toJSON() const {
  json::Object Root;
  Root["num_instructions"] = static_cast<int64_t>(NumInstructions);
  Root["num_dead_instructions"] = static_cast<int64_t>(NumDeadInstructions);
  Root["max_use_def_chain_depth"] = static_cast<int64_t>(MaxUseDefChainDepth);
  Root["mean_use_def_chain_depth"] = MeanUseDefChainDepth;
  return json::Value(std::move(Root));
}

void DeadCodeReport::print(raw_ostream &OS) const {
  OS << "dead code report: " << NumDeadInstructions << " of " << NumInstructions
     << " instructions trivially dead\n";
  OS << "  longest use-def chain: " << MaxUseDefChainDepth << '\n';
  OS << "  mean use-def chain: " << format("%.2f", MeanUseDefChainDepth)
     << '\n';
}

AnalysisKey DeadCodeAnalysis::Key;

DeadCodeReport DeadCodeAnalysis::run(Function &F, FunctionAnalysisManager &) {
  DeadCodeReport Report;
  for (const BasicBlock &BB : F)
    for (const Instruction &Inst : BB) {
      ++Report.NumInstructions;
      if (isTriviallyDead(Inst))
        ++Report.NumDeadInstructions;
    }

  ChainDepth Depth = useDefChainDepth(F);
  Report.MaxUseDefChainDepth = Depth.Max;
  Report.MeanUseDefChainDepth = Depth.Mean;
  return Report;
}

PreservedAnalyses DeadCodeReportPrinter::run(Function &F,
                                             FunctionAnalysisManager &AM) {
  const DeadCodeReport &Report = AM.getResult<DeadCodeAnalysis>(F);
  OS << "=== dead code report for function '" << F.getName() << "' ===\n";
  Report.print(OS);
  return PreservedAnalyses::all();
}

} // namespace opf
