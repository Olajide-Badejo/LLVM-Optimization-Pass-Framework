//===- metrics_json.cpp - one JSON document of all IR metrics ------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//

#include "opf/support/metrics_json.hpp"

#include "opf/analysis/block_stats.hpp"
#include "opf/analysis/dead_code_report.hpp"
#include "opf/analysis/opcode_stats.hpp"
#include "opf/analysis/simplify_opportunities.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opf {

PreservedAnalyses MetricsJSONEmitter::run(Module &M,
                                          ModuleAnalysisManager &MAM) {
  FunctionAnalysisManager &FAM =
      MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  json::Object Root;
  Root["module"] = M.getName();

  json::Array Functions;

  // Module wide rollups. Opcode counts merge across functions; block and
  // instruction counts sum.
  std::map<std::string, unsigned> TotalByOpcode;
  unsigned TotalInstructions = 0;
  unsigned TotalBlocks = 0;
  unsigned TotalUnreachable = 0;
  unsigned TotalDead = 0;
  unsigned TotalOpportunities = 0;
  unsigned NumDefinedFunctions = 0;

  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    ++NumDefinedFunctions;

    const OpcodeStats &Opcodes = FAM.getResult<OpcodeStatsAnalysis>(F);
    const BlockStats &Blocks = FAM.getResult<BlockStatsAnalysis>(F);
    const DeadCodeReport &Dead = FAM.getResult<DeadCodeAnalysis>(F);
    const SimplifyOpportunities &Opps =
        FAM.getResult<SimplifyOpportunitiesAnalysis>(F);

    json::Object FnObj;
    FnObj["name"] = F.getName();
    FnObj["opcode_stats"] = Opcodes.toJSON();
    FnObj["block_stats"] = Blocks.toJSON();
    FnObj["dead_code"] = Dead.toJSON();
    FnObj["simplify_opportunities"] = Opps.toJSON();
    Functions.push_back(std::move(FnObj));

    for (const auto &Entry : Opcodes.Counts)
      TotalByOpcode[Entry.first] += Entry.second;
    TotalInstructions += Opcodes.TotalInstructions;
    TotalBlocks += Blocks.NumBlocks;
    TotalUnreachable += Blocks.NumUnreachableBlocks;
    TotalDead += Dead.NumDeadInstructions;
    TotalOpportunities += Opps.total();
  }

  Root["functions"] = std::move(Functions);

  json::Object Totals;
  Totals["num_functions"] = static_cast<int64_t>(NumDefinedFunctions);
  Totals["total_instructions"] = static_cast<int64_t>(TotalInstructions);
  Totals["total_blocks"] = static_cast<int64_t>(TotalBlocks);
  Totals["total_unreachable_blocks"] = static_cast<int64_t>(TotalUnreachable);
  Totals["total_dead_instructions"] = static_cast<int64_t>(TotalDead);
  Totals["total_simplify_opportunities"] =
      static_cast<int64_t>(TotalOpportunities);
  json::Object ByOpcode;
  for (const auto &Entry : TotalByOpcode)
    ByOpcode[Entry.first] = static_cast<int64_t>(Entry.second);
  Totals["by_opcode"] = std::move(ByOpcode);
  Root["totals"] = std::move(Totals);

  // Two space indented so a human can read the harness output too.
  OS << formatv("{0:2}", json::Value(std::move(Root))) << '\n';
  return PreservedAnalyses::all();
}

} // namespace opf
