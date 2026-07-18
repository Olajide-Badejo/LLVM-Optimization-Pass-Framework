//===- plugin_registration.cpp - opf pass plugin entry point --------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// This is the single entry point a stock opt binary calls when it loads the
// plugin with -load-pass-plugin. Every analysis and transform the framework
// provides is wired into the new PassManager here, through PassBuilder
// callbacks, so pipelines compose from ordinary opt command lines and from
// named pipeline strings such as -passes="my-canon,my-simplify,my-dce".
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "opf/analysis/block_stats.hpp"
#include "opf/analysis/dead_code_report.hpp"
#include "opf/analysis/opcode_stats.hpp"
#include "opf/analysis/simplify_opportunities.hpp"
#include "opf/support/metrics_json.hpp"
#include "opf/transforms/algebraic_simplify.hpp"
#include "opf/transforms/canonicalize.hpp"
#include "opf/transforms/dce.hpp"
#include "opf/transforms/default_pipeline.hpp"
#include "opf/transforms/local_cse.hpp"

using namespace llvm;

namespace opf {

// A deliberately empty transform. It exists so that the Phase 0 gate can prove
// the plugin registers and runs a pass under a stock opt, before any real
// analysis or rewrite exists to muddy that signal. It preserves everything
// because it changes nothing.
struct NoOpPass : PassInfoMixin<NoOpPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    (void)F;
    return PreservedAnalyses::all();
  }

  static StringRef name() { return "my-noop"; }
};

// Function pass names. Returning true tells PassBuilder the name was consumed;
// false lets other plugins and the builtin parser have a turn.
static bool registerFunctionPass(StringRef Name, FunctionPassManager &FPM,
                                 ArrayRef<PassBuilder::PipelineElement>) {
  if (Name == "my-noop") {
    FPM.addPass(NoOpPass());
    return true;
  }
  if (Name == "my-print-opcode-stats") {
    FPM.addPass(OpcodeStatsPrinter(outs()));
    return true;
  }
  if (Name == "my-print-block-stats") {
    FPM.addPass(BlockStatsPrinter(outs()));
    return true;
  }
  if (Name == "my-print-dead-code") {
    FPM.addPass(DeadCodeReportPrinter(outs()));
    return true;
  }
  if (Name == "my-print-simplify-opps") {
    FPM.addPass(SimplifyOpportunitiesPrinter(outs()));
    return true;
  }
  if (Name == "my-canon") {
    FPM.addPass(CanonicalizePass());
    return true;
  }
  if (Name == "my-simplify") {
    FPM.addPass(AlgebraicSimplifyPass());
    return true;
  }
  if (Name == "my-local-cse") {
    FPM.addPass(LocalCSEPass());
    return true;
  }
  if (Name == "my-dce") {
    FPM.addPass(DCEPass());
    return true;
  }
  if (Name == "my-default") {
    FPM.addPass(DefaultPipelinePass());
    return true;
  }
  if (Name == "my-default-verbose") {
    // The iteration log goes to stderr so the transformed IR on stdout stays
    // clean for piping and for FileCheck.
    FPM.addPass(DefaultPipelinePass(&errs()));
    return true;
  }
  return false;
}

// Module pass names.
static bool registerModulePass(StringRef Name, ModulePassManager &MPM,
                               ArrayRef<PassBuilder::PipelineElement>) {
  if (Name == "my-metrics-json") {
    MPM.addPass(MetricsJSONEmitter(outs()));
    return true;
  }
  return false;
}

// Makes the framework's analyses available to any pass that asks the
// FunctionAnalysisManager for them.
static void registerAnalyses(FunctionAnalysisManager &FAM) {
  FAM.registerPass([] { return OpcodeStatsAnalysis(); });
  FAM.registerPass([] { return BlockStatsAnalysis(); });
  FAM.registerPass([] { return DeadCodeAnalysis(); });
  FAM.registerPass([] { return SimplifyOpportunitiesAnalysis(); });
}

static void registerCallbacks(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(
      [](StringRef Name, FunctionPassManager &FPM,
         ArrayRef<PassBuilder::PipelineElement> Pipeline) {
        return registerFunctionPass(Name, FPM, Pipeline);
      });
  PB.registerPipelineParsingCallback(
      [](StringRef Name, ModulePassManager &MPM,
         ArrayRef<PassBuilder::PipelineElement> Pipeline) {
        return registerModulePass(Name, MPM, Pipeline);
      });
  PB.registerAnalysisRegistrationCallback(
      [](FunctionAnalysisManager &FAM) { registerAnalyses(FAM); });
}

} // namespace opf

// The library information block opt reads on load. LLVM_PLUGIN_API_VERSION and
// LLVM_VERSION_STRING both come from the headers, so the plugin advertises the
// exact ABI and version it was compiled against.
llvm::PassPluginLibraryInfo getOPFPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "OPFPasses", LLVM_VERSION_STRING,
          opf::registerCallbacks};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOPFPluginInfo();
}
