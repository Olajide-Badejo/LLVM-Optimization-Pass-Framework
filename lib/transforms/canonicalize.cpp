//===- canonicalize.cpp - normalize commutative operand order ------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/transforms/canonicalize.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"

#include <limits>

using namespace llvm;

namespace opf {

unsigned canonicalizeFunction(Function &F) {
  // Number arguments, then instructions in program order. A lower number sorts
  // earlier; constants sort last so they land on the right hand side, which is
  // where the algebraic simplifier looks for them.
  DenseMap<const Value *, unsigned> Order;
  unsigned Index = 0;
  for (const Argument &Arg : F.args())
    Order[&Arg] = Index++;
  for (const BasicBlock &BB : F)
    for (const Instruction &Inst : BB)
      Order[&Inst] = Index++;

  const unsigned ConstantRank = std::numeric_limits<unsigned>::max();
  auto rankOf = [&](const Value *V) -> unsigned {
    if (isa<Constant>(V))
      return ConstantRank;
    auto It = Order.find(V);
    return It != Order.end() ? It->second : ConstantRank - 1;
  };

  unsigned Changes = 0;
  for (BasicBlock &BB : F)
    for (Instruction &Inst : BB) {
      if (!Inst.isCommutative() || Inst.getNumOperands() != 2)
        continue;
      Value *L = Inst.getOperand(0);
      Value *R = Inst.getOperand(1);
      // Swapping the operands of a commutative operation never changes its
      // result, so this is always safe.
      if (rankOf(L) > rankOf(R)) {
        Inst.setOperand(0, R);
        Inst.setOperand(1, L);
        ++Changes;
      }
    }
  return Changes;
}

PreservedAnalyses CanonicalizePass::run(Function &F,
                                        FunctionAnalysisManager &) {
  if (canonicalizeFunction(F) == 0)
    return PreservedAnalyses::all();
  // Only operand order changed, so the CFG and everything derived purely from
  // it survive. Value based analyses do not, and will recompute.
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

} // namespace opf
