//===- instruction_key.cpp - structural identity of an instruction -------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//

#include "opf/support/instruction_key.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opf {

bool isPureCandidate(const Instruction &Inst) {
  if (Inst.isTerminator() || isa<PHINode>(Inst))
    return false;
  if (Inst.getType()->isVoidTy() || Inst.getNumOperands() == 0)
    return false;
  if (Inst.mayHaveSideEffects() || Inst.mayReadFromMemory())
    return false;
  return true;
}

std::string structuralKey(const Instruction &Inst) {
  std::string Buffer;
  raw_string_ostream OS(Buffer);
  OS << Inst.getOpcode() << ':' << static_cast<const void *>(Inst.getType());

  SmallVector<const void *, 4> Ops;
  for (const Use &U : Inst.operands())
    Ops.push_back(static_cast<const void *>(U.get()));
  if (Inst.isCommutative() && Ops.size() == 2 && Ops[0] > Ops[1])
    std::swap(Ops[0], Ops[1]);
  for (const void *P : Ops)
    OS << ':' << P;

  if (const auto *Cmp = dyn_cast<CmpInst>(&Inst))
    OS << ":p" << static_cast<int>(Cmp->getPredicate());

  return OS.str();
}

} // namespace opf
