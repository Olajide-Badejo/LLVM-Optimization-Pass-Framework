//===- analysis_metrics_test.cpp - unit tests for analysis math ----------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// These tests parse hand written IR with known structure and assert the exact
// metrics each analysis reports, including the awkward cases: an empty
// declaration, a single block, an unreachable block, and duplicate
// computations. They exercise the analysis run methods and the free predicate
// functions the transforms will later share.
//
//===----------------------------------------------------------------------===//

#include "opf/analysis/block_stats.hpp"
#include "opf/analysis/dead_code_report.hpp"
#include "opf/analysis/opcode_stats.hpp"
#include "opf/analysis/simplify_opportunities.hpp"

#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/SourceMgr.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace opf;

namespace {

// A parsed module plus the analysis managers needed to run the passes. The
// managers are otherwise unused because these analyses do not depend on other
// analyses, but run() still needs one to take a reference to.
class MetricsTest : public ::testing::Test {
protected:
  LLVMContext Ctx;
  std::unique_ptr<Module> M;
  FunctionAnalysisManager FAM;

  Module &parse(const char *IR) {
    SMDiagnostic Err;
    M = parseAssemblyString(IR, Err, Ctx);
    if (!M) {
      std::string Msg;
      raw_string_ostream OS(Msg);
      Err.print("unit-test", OS);
      ADD_FAILURE() << "failed to parse IR: " << OS.str();
    }
    return *M;
  }

  Function &func(const char *Name) {
    Function *F = M->getFunction(Name);
    EXPECT_NE(F, nullptr) << "no function named " << Name;
    return *F;
  }

  // First instruction of the entry block, for predicate tests that plant the
  // instruction under test at the top of a function.
  Instruction &firstInst(const char *Name) {
    return *func(Name).getEntryBlock().begin();
  }
};

TEST_F(MetricsTest, OpcodeCountsAcrossBlocks) {
  parse(R"ir(
    define i32 @g(i32 %x, i32 %y) {
    entry:
      %a = add i32 %x, %y
      %b = mul i32 %a, %x
      %c = icmp sgt i32 %b, 0
      br i1 %c, label %t, label %e
    t:
      %d = sub i32 %b, %y
      br label %m
    e:
      %f = add i32 %b, %y
      br label %m
    m:
      %r = phi i32 [ %d, %t ], [ %f, %e ]
      ret i32 %r
    }
  )ir");

  OpcodeStats S = OpcodeStatsAnalysis().run(func("g"), FAM);
  EXPECT_EQ(S.TotalInstructions, 10u);
  EXPECT_EQ(S.countFor("add"), 2u);
  EXPECT_EQ(S.countFor("mul"), 1u);
  EXPECT_EQ(S.countFor("br"), 3u);
  EXPECT_EQ(S.countFor("phi"), 1u);
  EXPECT_EQ(S.countFor("ret"), 1u);
  EXPECT_EQ(S.countFor("nonexistent"), 0u);
  EXPECT_EQ(S.distinctOpcodes(), 7u);
}

TEST_F(MetricsTest, OpcodeEmptyDeclaration) {
  parse("declare i32 @ext(i32)");
  OpcodeStats S = OpcodeStatsAnalysis().run(func("ext"), FAM);
  EXPECT_EQ(S.TotalInstructions, 0u);
  EXPECT_EQ(S.distinctOpcodes(), 0u);
}

TEST_F(MetricsTest, BlockStatsSingleBlock) {
  parse(R"ir(
    define i32 @one(i32 %x) {
      %a = add i32 %x, 1
      ret i32 %a
    }
  )ir");
  BlockStats S = BlockStatsAnalysis().run(func("one"), FAM);
  EXPECT_EQ(S.NumBlocks, 1u);
  EXPECT_EQ(S.NumInstructions, 2u);
  EXPECT_EQ(S.MinBlockSize, 2u);
  EXPECT_EQ(S.MaxBlockSize, 2u);
  EXPECT_DOUBLE_EQ(S.MeanBlockSize, 2.0);
  EXPECT_EQ(S.NumUnreachableBlocks, 0u);
  EXPECT_EQ(S.TerminatorKinds.at("ret"), 1u);
}

TEST_F(MetricsTest, BlockStatsDiamondHistogramAndTerminators) {
  parse(R"ir(
    define void @diamond(i1 %c) {
    entry:
      br i1 %c, label %a, label %b
    a:
      br label %x
    b:
      br label %x
    x:
      ret void
    }
  )ir");
  BlockStats S = BlockStatsAnalysis().run(func("diamond"), FAM);
  EXPECT_EQ(S.NumBlocks, 4u);
  EXPECT_EQ(S.NumInstructions, 4u);
  EXPECT_EQ(S.TerminatorKinds.at("br"), 3u);
  EXPECT_EQ(S.TerminatorKinds.at("ret"), 1u);
  EXPECT_EQ(S.SizeHistogram.at(1u), 4u); // every block holds one instruction
  EXPECT_EQ(S.NumUnreachableBlocks, 0u);
}

TEST_F(MetricsTest, BlockStatsUnreachable) {
  parse(R"ir(
    define void @has_dead(i1 %c) {
    entry:
      br i1 %c, label %live, label %exit
    live:
      br label %exit
    dead:
      br label %exit
    exit:
      ret void
    }
  )ir");
  BlockStats S = BlockStatsAnalysis().run(func("has_dead"), FAM);
  EXPECT_EQ(S.NumUnreachableBlocks, 1u);
}

TEST_F(MetricsTest, DeadCodeCountsUnusedPureInstruction) {
  parse(R"ir(
    define i32 @dead(i32 %x) {
      %live = add i32 %x, 1
      %dead = mul i32 %x, %x
      ret i32 %live
    }
  )ir");
  DeadCodeReport R = DeadCodeAnalysis().run(func("dead"), FAM);
  EXPECT_EQ(R.NumInstructions, 3u);
  EXPECT_EQ(R.NumDeadInstructions, 1u); // %dead only
}

TEST_F(MetricsTest, DeadCodeStoreIsNotDead) {
  parse(R"ir(
    define void @with_store(ptr %p, i32 %x) {
      store i32 %x, ptr %p
      ret void
    }
  )ir");
  DeadCodeReport R = DeadCodeAnalysis().run(func("with_store"), FAM);
  EXPECT_EQ(R.NumDeadInstructions, 0u); // a store has side effects
}

TEST_F(MetricsTest, UseDefChainDepthStraightLine) {
  parse(R"ir(
    define i32 @chain(i32 %x) {
      %a = add i32 %x, 1
      %b = add i32 %a, 1
      %c = add i32 %b, 1
      ret i32 %c
    }
  )ir");
  ChainDepth D = useDefChainDepth(func("chain"));
  // a=1, b=2, c=3, ret depends on c so 4.
  EXPECT_EQ(D.Max, 4u);
  EXPECT_DOUBLE_EQ(D.Mean, 2.5);
}

TEST_F(MetricsTest, UseDefChainDepthTerminatesOnLoopPhi) {
  // A phi is a chain root, so the loop carried dependency does not loop
  // forever. This test passing at all is the real assertion.
  parse(R"ir(
    define void @loop(i32 %n) {
    entry:
      br label %head
    head:
      %i = phi i32 [ 0, %entry ], [ %next, %head ]
      %next = add i32 %i, 1
      %done = icmp eq i32 %next, %n
      br i1 %done, label %exit, label %head
    exit:
      ret void
    }
  )ir");
  ChainDepth D = useDefChainDepth(func("loop"));
  EXPECT_GT(D.Max, 0u);
}

TEST_F(MetricsTest, ConstantFoldableDetection) {
  parse(R"ir(
    define i32 @cf() { %r = add i32 3, 4  ret i32 %r }
    define i32 @nf(i32 %x) { %r = add i32 %x, 4  ret i32 %r }
  )ir");
  EXPECT_TRUE(isConstantFoldable(firstInst("cf")));
  EXPECT_FALSE(isConstantFoldable(firstInst("nf")));
}

TEST_F(MetricsTest, AlgebraicIdentityDetection) {
  parse(R"ir(
    define i32 @add0(i32 %x)  { %r = add i32 %x, 0    ret i32 %r }
    define i32 @mul1(i32 %x)  { %r = mul i32 %x, 1    ret i32 %r }
    define i32 @mul0(i32 %x)  { %r = mul i32 %x, 0    ret i32 %r }
    define i32 @subx(i32 %x)  { %r = sub i32 %x, %x   ret i32 %r }
    define i32 @orm1(i32 %x)  { %r = or  i32 %x, -1   ret i32 %r }
    define i32 @andm1(i32 %x) { %r = and i32 %x, -1   ret i32 %r }
    define i32 @xor0(i32 %x)  { %r = xor i32 %x, 0    ret i32 %r }
    define i32 @shl0(i32 %x)  { %r = shl i32 %x, 0    ret i32 %r }
    define i32 @plain(i32 %x) { %r = add i32 %x, 7    ret i32 %r }
  )ir");
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("add0")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("mul1")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("mul0")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("subx")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("orm1")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("andm1")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("xor0")));
  EXPECT_TRUE(isAlgebraicIdentity(firstInst("shl0")));
  EXPECT_FALSE(isAlgebraicIdentity(firstInst("plain")));
}

TEST_F(MetricsTest, StrengthReductionDetection) {
  parse(R"ir(
    define i32 @m8(i32 %x)  { %r = mul i32 %x, 8   ret i32 %r }
    define i32 @m1(i32 %x)  { %r = mul i32 %x, 1   ret i32 %r }
    define i32 @m6(i32 %x)  { %r = mul i32 %x, 6   ret i32 %r }
    define i32 @mc()        { %r = mul i32 4, 8    ret i32 %r }
  )ir");
  EXPECT_TRUE(isStrengthReducibleMul(firstInst("m8")));
  EXPECT_FALSE(isStrengthReducibleMul(firstInst("m1"))); // identity, not this
  EXPECT_FALSE(isStrengthReducibleMul(firstInst("m6"))); // not a power of two
  EXPECT_FALSE(isStrengthReducibleMul(firstInst("mc"))); // both constant
}

TEST_F(MetricsTest, DuplicatePureComputationsWithCommutativity) {
  parse(R"ir(
    define i32 @dup(i32 %x, i32 %y) {
      %a = add i32 %x, %y
      %b = add i32 %x, %y
      %c = add i32 %y, %x
      %d = mul i32 %a, %b
      ret i32 %d
    }
  )ir");
  // %b repeats %a, and %c is the commuted form of the same add, so two dupes.
  EXPECT_EQ(countDuplicatePureComputations(func("dup")), 2u);
}

TEST_F(MetricsTest, DuplicatesAreScopedPerBlock) {
  parse(R"ir(
    define i32 @two_blocks(i32 %x, i32 %y, i1 %c) {
    entry:
      %a = add i32 %x, %y
      br i1 %c, label %next, label %next
    next:
      %b = add i32 %x, %y
      ret i32 %b
    }
  )ir");
  // The add reappears in a different block, so it is not a duplicate here.
  EXPECT_EQ(countDuplicatePureComputations(func("two_blocks")), 0u);
}

TEST_F(MetricsTest, SimplifyOpportunitiesRollup) {
  parse(R"ir(
    define i32 @mix(i32 %x) {
      %id  = add i32 %x, 0
      %sr  = mul i32 %x, 8
      %cf  = add i32 2, 3
      ret i32 %id
    }
  )ir");
  SimplifyOpportunities O =
      SimplifyOpportunitiesAnalysis().run(func("mix"), FAM);
  EXPECT_EQ(O.NumAlgebraicIdentities, 1u);
  EXPECT_EQ(O.NumStrengthReducible, 1u);
  EXPECT_EQ(O.NumConstantFoldable, 1u);
}

} // namespace
