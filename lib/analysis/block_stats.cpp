//===- block_stats.cpp - basic block structure metrics -------------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//

#include "opf/analysis/block_stats.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opf {

json::Value BlockStats::toJSON() const {
  json::Object Root;
  Root["num_blocks"] = static_cast<int64_t>(NumBlocks);
  Root["num_instructions"] = static_cast<int64_t>(NumInstructions);
  Root["min_block_size"] = static_cast<int64_t>(MinBlockSize);
  Root["max_block_size"] = static_cast<int64_t>(MaxBlockSize);
  Root["mean_block_size"] = MeanBlockSize;
  Root["num_unreachable_blocks"] = static_cast<int64_t>(NumUnreachableBlocks);

  json::Object Hist;
  for (const auto &Entry : SizeHistogram)
    Hist[std::to_string(Entry.first)] = static_cast<int64_t>(Entry.second);
  Root["size_histogram"] = std::move(Hist);

  json::Object Terms;
  for (const auto &Entry : TerminatorKinds)
    Terms[Entry.first] = static_cast<int64_t>(Entry.second);
  Root["terminator_kinds"] = std::move(Terms);

  return json::Value(std::move(Root));
}

void BlockStats::print(raw_ostream &OS) const {
  OS << "block stats: " << NumBlocks << " blocks, " << NumInstructions
     << " instructions\n";
  OS << "  size min/mean/max: " << MinBlockSize << " / ";
  OS << format("%.2f", MeanBlockSize) << " / " << MaxBlockSize << '\n';
  OS << "  unreachable blocks: " << NumUnreachableBlocks << '\n';
  OS << "  terminators:\n";
  for (const auto &Entry : TerminatorKinds)
    OS << "    " << format_decimal(Entry.second, 6) << "  " << Entry.first
       << '\n';
  OS << "  size histogram (block size: count):\n";
  for (const auto &Entry : SizeHistogram)
    OS << "    " << format_decimal(Entry.first, 4) << ": " << Entry.second
       << '\n';
}

AnalysisKey BlockStatsAnalysis::Key;

BlockStats BlockStatsAnalysis::run(Function &F, FunctionAnalysisManager &) {
  BlockStats Stats;
  if (F.isDeclaration())
    return Stats;

  const BasicBlock *Entry = &F.getEntryBlock();
  unsigned RunningTotal = 0;
  bool First = true;

  for (const BasicBlock &BB : F) {
    const unsigned Size = static_cast<unsigned>(BB.size());
    ++Stats.NumBlocks;
    Stats.NumInstructions += Size;
    RunningTotal += Size;
    ++Stats.SizeHistogram[Size];

    if (First || Size < Stats.MinBlockSize)
      Stats.MinBlockSize = Size;
    if (First || Size > Stats.MaxBlockSize)
      Stats.MaxBlockSize = Size;
    First = false;

    if (const auto *Term = BB.getTerminator())
      ++Stats.TerminatorKinds[Term->getOpcodeName()];

    // Entry legitimately has no predecessors; any other predecessor free block
    // is unreachable.
    if (&BB != Entry && pred_empty(&BB))
      ++Stats.NumUnreachableBlocks;
  }

  if (Stats.NumBlocks)
    Stats.MeanBlockSize = static_cast<double>(RunningTotal) /
                          static_cast<double>(Stats.NumBlocks);
  return Stats;
}

PreservedAnalyses BlockStatsPrinter::run(Function &F,
                                         FunctionAnalysisManager &AM) {
  const BlockStats &Stats = AM.getResult<BlockStatsAnalysis>(F);
  OS << "=== block stats for function '" << F.getName() << "' ===\n";
  Stats.print(OS);
  return PreservedAnalyses::all();
}

} // namespace opf
