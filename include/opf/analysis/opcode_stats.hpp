//===- opcode_stats.hpp - per opcode instruction counts -------------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//
//
// A new PassManager function analysis that counts instructions by opcode, plus
// a printer pass for human output. The result is a plain value struct so unit
// tests can assert on the numbers and the JSON emitter can serialize them for
// the benchmark harness.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_ANALYSIS_OPCODE_STATS_HPP
#define OPF_ANALYSIS_OPCODE_STATS_HPP

#include <map>
#include <string>

#include "llvm/IR/PassManager.h"
#include "llvm/Support/JSON.h"

namespace llvm {
class Function;
class Instruction;
class raw_ostream;
} // namespace llvm

namespace opf {

// Instruction counts keyed by LLVM opcode mnemonic (the string
// Instruction::getOpcodeName returns, for example "add" or "getelementptr").
// The map is ordered so printed and serialized output is deterministic.
struct OpcodeStats {
  std::map<std::string, unsigned> Counts;
  unsigned TotalInstructions = 0;

  // Folds one instruction into the tally.
  void addInstruction(const llvm::Instruction &Inst);

  // Count for a given mnemonic, or zero if the opcode never appeared.
  unsigned countFor(llvm::StringRef Opcode) const;

  // Number of distinct opcodes seen.
  unsigned distinctOpcodes() const {
    return static_cast<unsigned>(Counts.size());
  }

  llvm::json::Value toJSON() const;
  void print(llvm::raw_ostream &OS) const;
};

// The analysis. Computing it is a linear walk of the function's instructions.
class OpcodeStatsAnalysis
    : public llvm::AnalysisInfoMixin<OpcodeStatsAnalysis> {
  friend llvm::AnalysisInfoMixin<OpcodeStatsAnalysis>;
  static llvm::AnalysisKey Key;

public:
  using Result = OpcodeStats;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

// Human readable printer, registered under the pipeline name
// "my-print-opcode-stats".
class OpcodeStatsPrinter : public llvm::PassInfoMixin<OpcodeStatsPrinter> {
  llvm::raw_ostream &OS;

public:
  explicit OpcodeStatsPrinter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace opf

#endif // OPF_ANALYSIS_OPCODE_STATS_HPP
