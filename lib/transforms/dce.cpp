//===- dce.cpp - dead code elimination -----------------------------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//

#include "opf/transforms/dce.hpp"

#include "opf/analysis/dead_code_report.hpp"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

namespace opf {

unsigned dceFunction(Function &F) {
  // Seed the worklist with everything currently dead. The InWorklist set keeps
  // each instruction queued at most once so no pointer is ever freed twice.
  SmallVector<Instruction *, 32> Worklist;
  SmallPtrSet<Instruction *, 32> InWorklist;
  for (BasicBlock &BB : F)
    for (Instruction &Inst : BB)
      if (isTriviallyDead(Inst) && InWorklist.insert(&Inst).second)
        Worklist.push_back(&Inst);

  unsigned Removed = 0;
  while (!Worklist.empty()) {
    Instruction *Inst = Worklist.pop_back_val();
    InWorklist.erase(Inst);

    // Remember the instruction operands before erasing, since removing this use
    // may be what makes one of them dead in turn.
    SmallVector<Instruction *, 4> Operands;
    for (Use &U : Inst->operands())
      if (auto *OpInst = dyn_cast<Instruction>(U.get()))
        Operands.push_back(OpInst);

    Inst->eraseFromParent();
    ++Removed;

    for (Instruction *OpInst : Operands)
      if (isTriviallyDead(*OpInst) && InWorklist.insert(OpInst).second)
        Worklist.push_back(OpInst);
  }
  return Removed;
}

PreservedAnalyses DCEPass::run(Function &F, FunctionAnalysisManager &) {
  if (dceFunction(F) == 0)
    return PreservedAnalyses::all();
  // Only non terminator instructions are removed, so the CFG is intact.
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

} // namespace opf
