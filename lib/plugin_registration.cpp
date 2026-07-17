//===- plugin_registration.cpp - opf pass plugin entry point --------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
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

  // Even passes registered purely through the pipeline parser benefit from a
  // stable name for logs and for opt's -debug-pass-manager output.
  static StringRef name() { return "my-noop"; }
};

// Registers the framework's function passes on a pipeline parsing callback.
// Returning true tells PassBuilder the name was consumed; false lets other
// plugins and the builtin parser have a turn.
static bool registerFunctionPass(StringRef Name, FunctionPassManager &FPM,
                                 ArrayRef<PassBuilder::PipelineElement>) {
  if (Name == "my-noop") {
    FPM.addPass(NoOpPass());
    return true;
  }
  return false;
}

static void registerCallbacks(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(
      [](StringRef Name, FunctionPassManager &FPM,
         ArrayRef<PassBuilder::PipelineElement> Pipeline) {
        return registerFunctionPass(Name, FPM, Pipeline);
      });
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
