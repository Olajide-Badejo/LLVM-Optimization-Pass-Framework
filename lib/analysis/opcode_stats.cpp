//===- opcode_stats.cpp - per opcode instruction counts -------------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//

#include "opf/analysis/opcode_stats.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opf {

void OpcodeStats::addInstruction(const Instruction &Inst) {
  ++Counts[Inst.getOpcodeName()];
  ++TotalInstructions;
}

unsigned OpcodeStats::countFor(StringRef Opcode) const {
  auto It = Counts.find(std::string(Opcode));
  return It == Counts.end() ? 0u : It->second;
}

json::Value OpcodeStats::toJSON() const {
  json::Object Root;
  Root["total_instructions"] = static_cast<int64_t>(TotalInstructions);
  Root["distinct_opcodes"] = static_cast<int64_t>(distinctOpcodes());
  json::Object ByOpcode;
  for (const auto &Entry : Counts)
    ByOpcode[Entry.first] = static_cast<int64_t>(Entry.second);
  Root["by_opcode"] = std::move(ByOpcode);
  return json::Value(std::move(Root));
}

void OpcodeStats::print(raw_ostream &OS) const {
  OS << "opcode stats: " << TotalInstructions << " instructions across "
     << distinctOpcodes() << " opcodes\n";
  for (const auto &Entry : Counts)
    OS << "  " << format_decimal(Entry.second, 6) << "  " << Entry.first
       << '\n';
}

AnalysisKey OpcodeStatsAnalysis::Key;

OpcodeStats OpcodeStatsAnalysis::run(Function &F, FunctionAnalysisManager &) {
  OpcodeStats Stats;
  for (const BasicBlock &BB : F)
    for (const Instruction &Inst : BB)
      Stats.addInstruction(Inst);
  return Stats;
}

PreservedAnalyses OpcodeStatsPrinter::run(Function &F,
                                          FunctionAnalysisManager &AM) {
  const OpcodeStats &Stats = AM.getResult<OpcodeStatsAnalysis>(F);
  OS << "=== opcode stats for function '" << F.getName() << "' ===\n";
  Stats.print(OS);
  return PreservedAnalyses::all();
}

} // namespace opf
