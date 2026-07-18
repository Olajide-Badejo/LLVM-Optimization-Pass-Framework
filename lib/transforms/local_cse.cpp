//===- local_cse.cpp - block local common subexpression cleanup ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/transforms/local_cse.hpp"

#include "opf/support/instruction_key.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

namespace opf {

unsigned localCSEFunction(Function &F) {
  unsigned Removed = 0;
  for (BasicBlock &BB : F) {
    // First occurrence of each computation, keyed structurally. The map is
    // scoped to the block, which is what makes this local CSE.
    StringMap<Instruction *> FirstSeen;
    SmallVector<Instruction *, 16> ToErase;

    for (Instruction &Inst : BB) {
      if (!isPureCandidate(Inst))
        continue;
      std::string Key = structuralKey(Inst);
      auto It = FirstSeen.find(Key);
      if (It != FirstSeen.end()) {
        // Redirect this duplicate to the earlier instruction, which dominates
        // it because it sits earlier in the same block.
        Inst.replaceAllUsesWith(It->second);
        ToErase.push_back(&Inst);
        ++Removed;
      } else {
        FirstSeen[Key] = &Inst;
      }
    }

    for (Instruction *Inst : ToErase)
      Inst->eraseFromParent();
  }
  return Removed;
}

PreservedAnalyses LocalCSEPass::run(Function &F, FunctionAnalysisManager &) {
  if (localCSEFunction(F) == 0)
    return PreservedAnalyses::all();
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

} // namespace opf
