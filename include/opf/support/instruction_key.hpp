//===- instruction_key.hpp - structural identity of an instruction -------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//
//
// Shared helpers for deciding when two instructions compute the same value.
// Both the simplify opportunities analysis (which only counts duplicates) and
// the local CSE transform (which removes them) use these, so the two can never
// disagree about what counts as a duplicate.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_SUPPORT_INSTRUCTION_KEY_HPP
#define OPF_SUPPORT_INSTRUCTION_KEY_HPP

#include <string>

namespace llvm {
class Instruction;
} // namespace llvm

namespace opf {

// A pure, value producing computation worth deduplicating. Loads are excluded
// because two identical loads can observe different memory; anything with side
// effects, terminators, and phi nodes are excluded too.
bool isPureCandidate(const llvm::Instruction &Inst);

// A string that is equal for two instructions exactly when they compute the
// same value from the same operands. Commutative operands are ordered so that
// add a b and add b a share a key. Operand identity is by SSA value, so this is
// meaningful only within a single module in a single run, which is all local
// CSE and the duplicate count need.
std::string structuralKey(const llvm::Instruction &Inst);

} // namespace opf

#endif // OPF_SUPPORT_INSTRUCTION_KEY_HPP
