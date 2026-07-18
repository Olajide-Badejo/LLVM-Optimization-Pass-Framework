//===- simplify_opportunities.cpp - count simplifiable patterns ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/analysis/simplify_opportunities.hpp"

#include "opf/support/instruction_key.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>

using namespace llvm;

namespace opf {

// Returns the operand as a ConstantInt, or null.
static const ConstantInt *asConstInt(const Value *V) {
  return dyn_cast<ConstantInt>(V);
}

bool isConstantFoldable(const Instruction &Inst) {
  const auto *BinOp = dyn_cast<BinaryOperator>(&Inst);
  if (!BinOp)
    return false;
  return asConstInt(BinOp->getOperand(0)) && asConstInt(BinOp->getOperand(1));
}

bool isAlgebraicIdentity(const Instruction &Inst) {
  const auto *BinOp = dyn_cast<BinaryOperator>(&Inst);
  if (!BinOp)
    return false;

  const Value *L = BinOp->getOperand(0);
  const Value *R = BinOp->getOperand(1);
  const ConstantInt *CL = asConstInt(L);
  const ConstantInt *CR = asConstInt(R);
  const bool SameOperand = (L == R);

  switch (BinOp->getOpcode()) {
  case Instruction::Add:
    // x + 0
    return (CR && CR->isZero()) || (CL && CL->isZero());
  case Instruction::Sub:
    // x - 0, or x - x
    return (CR && CR->isZero()) || SameOperand;
  case Instruction::Mul:
    // x * 1, x * 0
    return (CR && (CR->isOne() || CR->isZero())) ||
           (CL && (CL->isOne() || CL->isZero()));
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    // x shift 0
    return CR && CR->isZero();
  case Instruction::And:
    // x & 0, x & -1, x & x
    return (CR && (CR->isZero() || CR->isMinusOne())) ||
           (CL && (CL->isZero() || CL->isMinusOne())) || SameOperand;
  case Instruction::Or:
    // x | 0, x | -1, x | x
    return (CR && (CR->isZero() || CR->isMinusOne())) ||
           (CL && (CL->isZero() || CL->isMinusOne())) || SameOperand;
  case Instruction::Xor:
    // x ^ 0, x ^ x
    return (CR && CR->isZero()) || (CL && CL->isZero()) || SameOperand;
  default:
    return false;
  }
}

bool isStrengthReducibleMul(const Instruction &Inst) {
  const auto *BinOp = dyn_cast<BinaryOperator>(&Inst);
  if (!BinOp || BinOp->getOpcode() != Instruction::Mul)
    return false;

  const Value *L = BinOp->getOperand(0);
  const Value *R = BinOp->getOperand(1);
  const ConstantInt *CL = asConstInt(L);
  const ConstantInt *CR = asConstInt(R);

  // Exactly one side is a constant power of two above one; the other stays
  // symbolic. Two constants would be constant folding, and by one is identity.
  auto powerOfTwoAboveOne = [](const ConstantInt *C) {
    return C && !C->isOne() && C->getValue().isPowerOf2();
  };
  if (CL && CR)
    return false;
  return powerOfTwoAboveOne(CR) || powerOfTwoAboveOne(CL);
}

unsigned countDuplicatePureComputations(const Function &F) {
  // The predicate and key come from the shared instruction_key module, the same
  // ones local CSE uses to actually remove these duplicates.
  unsigned Duplicates = 0;
  for (const BasicBlock &BB : F) {
    std::set<std::string> Seen;
    for (const Instruction &Inst : BB) {
      if (!isPureCandidate(Inst))
        continue;
      if (!Seen.insert(structuralKey(Inst)).second)
        ++Duplicates;
    }
  }
  return Duplicates;
}

json::Value SimplifyOpportunities::toJSON() const {
  json::Object Root;
  Root["constant_foldable"] = static_cast<int64_t>(NumConstantFoldable);
  Root["algebraic_identities"] = static_cast<int64_t>(NumAlgebraicIdentities);
  Root["strength_reducible"] = static_cast<int64_t>(NumStrengthReducible);
  Root["duplicate_pure_computations"] =
      static_cast<int64_t>(NumDuplicatePureComputations);
  Root["total"] = static_cast<int64_t>(total());
  return json::Value(std::move(Root));
}

void SimplifyOpportunities::print(raw_ostream &OS) const {
  OS << "simplify opportunities: " << total() << " total\n";
  OS << "  constant foldable:   " << NumConstantFoldable << '\n';
  OS << "  algebraic identity:  " << NumAlgebraicIdentities << '\n';
  OS << "  strength reducible:  " << NumStrengthReducible << '\n';
  OS << "  duplicate pure:      " << NumDuplicatePureComputations << '\n';
}

AnalysisKey SimplifyOpportunitiesAnalysis::Key;

SimplifyOpportunities
SimplifyOpportunitiesAnalysis::run(Function &F, FunctionAnalysisManager &) {
  SimplifyOpportunities Opps;
  for (const BasicBlock &BB : F)
    for (const Instruction &Inst : BB) {
      if (isConstantFoldable(Inst))
        ++Opps.NumConstantFoldable;
      if (isAlgebraicIdentity(Inst))
        ++Opps.NumAlgebraicIdentities;
      if (isStrengthReducibleMul(Inst))
        ++Opps.NumStrengthReducible;
    }
  Opps.NumDuplicatePureComputations = countDuplicatePureComputations(F);
  return Opps;
}

PreservedAnalyses
SimplifyOpportunitiesPrinter::run(Function &F, FunctionAnalysisManager &AM) {
  const SimplifyOpportunities &Opps =
      AM.getResult<SimplifyOpportunitiesAnalysis>(F);
  OS << "=== simplify opportunities for function '" << F.getName() << "' ===\n";
  Opps.print(OS);
  return PreservedAnalyses::all();
}

} // namespace opf
