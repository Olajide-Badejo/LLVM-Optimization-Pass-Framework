//===- default_pipeline.hpp - the my-default fixed point pipeline --------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// The my-default pipeline runs canonicalize, algebraic simplify, local CSE, and
// DCE in that order, repeating until an iteration makes no change or a cap is
// reached. The order is deliberate: canonicalization exposes duplicates for
// CSE, simplification creates dead code, and DCE cleans up, which can expose
// more for the next round. The driver is a free function so the integration
// test can inspect the iteration log directly.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_TRANSFORMS_DEFAULT_PIPELINE_HPP
#define OPF_TRANSFORMS_DEFAULT_PIPELINE_HPP

#include "opf/support/pipeline_logging.hpp"

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
class raw_ostream;
} // namespace llvm

namespace opf {

// The default iteration cap. Reaching it means the pipeline stopped before a
// fixed point, which the log records honestly.
inline constexpr unsigned DEFAULT_PIPELINE_MAX_ITERATIONS = 8;

// Runs the pipeline to a fixed point (or the cap), filling Log with per
// iteration detail. Returns the total number of rewrites across all iterations.
unsigned runDefaultPipeline(llvm::Function &F, PipelineLog &Log,
                            unsigned MaxIterations);

// Registered under the pipeline names "my-default" (quiet) and
// "my-default-verbose" (prints the iteration log to stderr).
class DefaultPipelinePass : public llvm::PassInfoMixin<DefaultPipelinePass> {
  llvm::raw_ostream *LogOS;
  unsigned MaxIterations;

public:
  explicit DefaultPipelinePass(
      llvm::raw_ostream *LogOS = nullptr,
      unsigned MaxIterations = DEFAULT_PIPELINE_MAX_ITERATIONS)
      : LogOS(LogOS), MaxIterations(MaxIterations) {}

  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

} // namespace opf

#endif // OPF_TRANSFORMS_DEFAULT_PIPELINE_HPP
