//===- algebraic_simplify.cpp - fold identities and strength reduce ------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/transforms/algebraic_simplify.hpp"

#include "llvm/IR/Analysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace opf {

// Returns the value Inst should be replaced with, or null if no rewrite
// applies. The returned value is either an existing operand, a fresh constant,
// or a newly created shift inserted before Inst.
static Value *trySimplify(Instruction &Inst) {
  auto *BinOp = dyn_cast<BinaryOperator>(&Inst);
  if (!BinOp)
    return nullptr;

  // Scalar integers only. This keeps every rewrite below trivially value
  // preserving and avoids floating point identities (which need fast math
  // flags) and vector splat corner cases.
  Type *Ty = BinOp->getType();
  if (!Ty->isIntegerTy())
    return nullptr;

  Value *L = BinOp->getOperand(0);
  Value *R = BinOp->getOperand(1);
  auto *CL = dyn_cast<ConstantInt>(L);
  auto *CR = dyn_cast<ConstantInt>(R);
  const bool Same = (L == R);

  auto zero = [&] { return ConstantInt::get(Ty, 0); };

  auto strengthReduce = [&](Value *X, const ConstantInt *C) -> Value * {
    // mul X, 2^k  becomes  shl X, k, carrying the wrap flags forward, which is
    // valid because the shift produces the same bits and overflows exactly when
    // the multiply did.
    unsigned Shift = C->getValue().logBase2();
    IRBuilder<> Builder(BinOp);
    Value *Shl = Builder.CreateShl(X, ConstantInt::get(Ty, Shift));
    if (auto *ShlInst = dyn_cast<BinaryOperator>(Shl)) {
      ShlInst->setHasNoUnsignedWrap(BinOp->hasNoUnsignedWrap());
      ShlInst->setHasNoSignedWrap(BinOp->hasNoSignedWrap());
    }
    return Shl;
  };

  switch (BinOp->getOpcode()) {
  case Instruction::Add:
    if (CR && CR->isZero())
      return L;
    if (CL && CL->isZero())
      return R;
    break;
  case Instruction::Sub:
    if (CR && CR->isZero())
      return L;
    if (Same)
      return zero();
    break;
  case Instruction::Mul:
    if ((CR && CR->isZero()) || (CL && CL->isZero()))
      return zero();
    if (CR && CR->isOne())
      return L;
    if (CL && CL->isOne())
      return R;
    if (CR && !CR->isOne() && CR->getValue().isPowerOf2())
      return strengthReduce(L, CR);
    if (CL && !CL->isOne() && CL->getValue().isPowerOf2())
      return strengthReduce(R, CL);
    break;
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    if (CR && CR->isZero())
      return L;
    break;
  case Instruction::And:
    if ((CR && CR->isZero()) || (CL && CL->isZero()))
      return zero();
    if (CR && CR->isMinusOne())
      return L;
    if (CL && CL->isMinusOne())
      return R;
    if (Same)
      return L;
    break;
  case Instruction::Or:
    if (CR && CR->isMinusOne())
      return CR;
    if (CL && CL->isMinusOne())
      return CL;
    if (CR && CR->isZero())
      return L;
    if (CL && CL->isZero())
      return R;
    if (Same)
      return L;
    break;
  case Instruction::Xor:
    if (CR && CR->isZero())
      return L;
    if (CL && CL->isZero())
      return R;
    if (Same)
      return zero();
    break;
  default:
    break;
  }
  return nullptr;
}

unsigned algebraicSimplifyFunction(Function &F) {
  unsigned Changes = 0;
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (BasicBlock &BB : F) {
      for (auto It = BB.begin(); It != BB.end();) {
        Instruction &Inst = *It++;
        Value *Replacement = trySimplify(Inst);
        if (!Replacement || Replacement == &Inst)
          continue;
        Inst.replaceAllUsesWith(Replacement);
        // The rewritten instruction now has no uses and, being an integer
        // binary operator, no side effects, so it is safe to erase.
        Inst.eraseFromParent();
        ++Changes;
        Changed = true;
      }
    }
  }
  return Changes;
}

PreservedAnalyses AlgebraicSimplifyPass::run(Function &F,
                                             FunctionAnalysisManager &) {
  if (algebraicSimplifyFunction(F) == 0)
    return PreservedAnalyses::all();
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

} // namespace opf
