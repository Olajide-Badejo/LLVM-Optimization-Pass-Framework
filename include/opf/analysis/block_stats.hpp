//===- block_stats.hpp - basic block structure metrics --------------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// A new PassManager function analysis over basic block structure: how many
// blocks, how large they are (a size histogram plus min, max, mean), and which
// terminator opcodes end them. The result feeds a human printer and the JSON
// emitter.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_ANALYSIS_BLOCK_STATS_HPP
#define OPF_ANALYSIS_BLOCK_STATS_HPP

#include <map>
#include <string>

#include "llvm/IR/PassManager.h"
#include "llvm/Support/JSON.h"

namespace llvm {
class Function;
class raw_ostream;
} // namespace llvm

namespace opf {

struct BlockStats {
  unsigned NumBlocks = 0;
  unsigned NumInstructions = 0;
  unsigned MinBlockSize = 0;
  unsigned MaxBlockSize = 0;
  double MeanBlockSize = 0.0;

  // Block instruction count -> number of blocks with that count.
  std::map<unsigned, unsigned> SizeHistogram;

  // Terminator opcode mnemonic (for example "ret", "br", "switch") -> count.
  std::map<std::string, unsigned> TerminatorKinds;

  // Blocks with no predecessors other than function entry. A well formed
  // function has exactly one such block (entry); more means unreachable code.
  unsigned NumUnreachableBlocks = 0;

  llvm::json::Value toJSON() const;
  void print(llvm::raw_ostream &OS) const;
};

class BlockStatsAnalysis : public llvm::AnalysisInfoMixin<BlockStatsAnalysis> {
  friend llvm::AnalysisInfoMixin<BlockStatsAnalysis>;
  static llvm::AnalysisKey Key;

public:
  using Result = BlockStats;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

// Registered under the pipeline name "my-print-block-stats".
class BlockStatsPrinter : public llvm::PassInfoMixin<BlockStatsPrinter> {
  llvm::raw_ostream &OS;

public:
  explicit BlockStatsPrinter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace opf

#endif // OPF_ANALYSIS_BLOCK_STATS_HPP
