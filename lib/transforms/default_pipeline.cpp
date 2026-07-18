//===- default_pipeline.cpp - the my-default fixed point pipeline --------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/transforms/default_pipeline.hpp"

#include "opf/transforms/algebraic_simplify.hpp"
#include "opf/transforms/canonicalize.hpp"
#include "opf/transforms/dce.hpp"
#include "opf/transforms/local_cse.hpp"

#include "llvm/IR/Analysis.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opf {

unsigned runDefaultPipeline(Function &F, PipelineLog &Log,
                            unsigned MaxIterations) {
  Log.setSubject(F.getName());

  unsigned Total = 0;
  for (unsigned It = 0; It < MaxIterations; ++It) {
    IterationLog &Iteration = Log.newIteration();

    // The order is the whole point of the pipeline; see the header.
    Iteration.Runs.push_back({"my-canon", canonicalizeFunction(F)});
    Iteration.Runs.push_back({"my-simplify", algebraicSimplifyFunction(F)});
    Iteration.Runs.push_back({"my-local-cse", localCSEFunction(F)});
    Iteration.Runs.push_back({"my-dce", dceFunction(F)});

    Total += Iteration.total();

    // An iteration that changed nothing means we have reached a fixed point.
    if (Iteration.total() == 0) {
      Log.markReachedCap(false);
      return Total;
    }
  }

  // Exhausted the cap while still making progress.
  Log.markReachedCap(true);
  return Total;
}

PreservedAnalyses DefaultPipelinePass::run(Function &F,
                                           FunctionAnalysisManager &) {
  PipelineLog Log;
  unsigned Total = runDefaultPipeline(F, Log, MaxIterations);
  if (LogOS)
    Log.print(*LogOS);
  if (Total == 0)
    return PreservedAnalyses::all();
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

} // namespace opf
