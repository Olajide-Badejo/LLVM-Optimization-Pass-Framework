//===- invalidation_test.cpp - analysis invalidation and pipeline --------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// Two learning goals live here. First, that a transform which mutates the IR
// forces a dependent analysis to recompute rather than handing back a stale
// cached result: the whole point of getting PreservedAnalyses right. Second,
// that the my-default pipeline runs its passes in the intended order, reaches a
// fixed point, and honors the iteration cap.
//
//===----------------------------------------------------------------------===//

#include "opf/analysis/opcode_stats.hpp"
#include "opf/support/pipeline_logging.hpp"
#include "opf/transforms/algebraic_simplify.hpp"
#include "opf/transforms/default_pipeline.hpp"

#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/SourceMgr.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace opf;

namespace {

std::unique_ptr<Module> parseIR(LLVMContext &Ctx, const char *IR) {
  SMDiagnostic Err;
  auto M = parseAssemblyString(IR, Err, Ctx);
  EXPECT_TRUE(M != nullptr);
  return M;
}

// Wires up the full set of analysis managers the way opt does, then registers
// the framework's function analyses on top.
struct Managers {
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;

  Managers() {
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    FAM.registerPass([] { return OpcodeStatsAnalysis(); });
  }
};

TEST(Invalidation, TransformForcesAnalysisRecompute) {
  LLVMContext Ctx;
  auto M = parseIR(Ctx, R"ir(
    define i32 @f(i32 %x) {
      %a = add i32 %x, 0
      %b = add i32 %a, 0
      %c = mul i32 %b, 1
      ret i32 %c
    }
  )ir");
  Function &F = *M->getFunction("f");

  Managers Mgr;

  // Cache the analysis result. Four instructions before any transform.
  unsigned Before = Mgr.FAM.getResult<OpcodeStatsAnalysis>(F).TotalInstructions;
  EXPECT_EQ(Before, 4u);

  // A transform that shrinks the function and preserves only the CFG. Running
  // it through a pass manager triggers the invalidation machinery.
  FunctionPassManager FPM;
  FPM.addPass(AlgebraicSimplifyPass());
  FPM.run(F, Mgr.FAM);

  // If invalidation is wrong, this hands back the stale 4. It must recompute.
  unsigned After = Mgr.FAM.getResult<OpcodeStatsAnalysis>(F).TotalInstructions;
  EXPECT_LT(After, Before);
  EXPECT_EQ(After, 1u); // only the ret survives
}

TEST(Pipeline, ReachesFixedPointInOrder) {
  LLVMContext Ctx;
  auto M = parseIR(Ctx, R"ir(
    define i32 @composite(i32 %x, i32 %y) {
      %a = add i32 %x, 0
      %b = add i32 0, %x
      %c = mul i32 %a, 4
      %d = mul i32 %b, 4
      %e = add i32 %c, %d
      %dead = xor i32 %y, %y
      ret i32 %e
    }
  )ir");
  Function &F = *M->getFunction("composite");

  PipelineLog Log;
  unsigned Total = runDefaultPipeline(F, Log, DEFAULT_PIPELINE_MAX_ITERATIONS);

  EXPECT_GT(Total, 0u);
  EXPECT_TRUE(Log.reachedFixedPoint());

  ASSERT_FALSE(Log.iterations().empty());
  const IterationLog &First = Log.iterations().front();
  ASSERT_EQ(First.Runs.size(), 4u);
  EXPECT_EQ(First.Runs[0].Pass, "my-canon");
  EXPECT_EQ(First.Runs[1].Pass, "my-simplify");
  EXPECT_EQ(First.Runs[2].Pass, "my-local-cse");
  EXPECT_EQ(First.Runs[3].Pass, "my-dce");

  // The final iteration is the one that changed nothing, confirming the point.
  EXPECT_EQ(Log.iterations().back().total(), 0u);
}

TEST(Pipeline, RespectsIterationCap) {
  LLVMContext Ctx;
  auto M = parseIR(Ctx, R"ir(
    define i32 @composite(i32 %x, i32 %y) {
      %a = add i32 %x, 0
      %b = add i32 0, %x
      %c = mul i32 %a, 4
      %d = mul i32 %b, 4
      %e = add i32 %c, %d
      %dead = xor i32 %y, %y
      ret i32 %e
    }
  )ir");
  Function &F = *M->getFunction("composite");

  // A cap of one stops before the confirming zero change iteration, so the log
  // honestly reports it did not reach a fixed point.
  PipelineLog Log;
  runDefaultPipeline(F, Log, 1);
  EXPECT_EQ(Log.numIterations(), 1u);
  EXPECT_FALSE(Log.reachedFixedPoint());
}

} // namespace
