//===- pipeline_logging.cpp - per iteration pipeline record --------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include "opf/support/pipeline_logging.hpp"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opf {

unsigned IterationLog::total() const {
  unsigned Sum = 0;
  for (const PassRun &Run : Runs)
    Sum += Run.Rewrites;
  return Sum;
}

void PipelineLog::setSubject(StringRef Name) { Subject = Name.str(); }

IterationLog &PipelineLog::newIteration() {
  Iterations.emplace_back();
  Iterations.back().Index = static_cast<unsigned>(Iterations.size());
  return Iterations.back();
}

unsigned PipelineLog::totalRewrites() const {
  unsigned Sum = 0;
  for (const IterationLog &It : Iterations)
    Sum += It.total();
  return Sum;
}

void PipelineLog::print(raw_ostream &OS) const {
  OS << "=== my-default pipeline on '" << Subject << "' ===\n";
  for (const IterationLog &It : Iterations) {
    OS << "iteration " << It.Index << ":";
    for (const PassRun &Run : It.Runs)
      OS << ' ' << Run.Pass << '=' << Run.Rewrites;
    OS << " (total " << It.total() << ")\n";
  }
  if (reachedFixedPoint())
    OS << "reached fixed point after " << numIterations() << " iterations, "
       << totalRewrites() << " rewrites total\n";
  else
    OS << "stopped at iteration cap (" << numIterations() << "), "
       << totalRewrites() << " rewrites total; more may remain\n";
}

} // namespace opf
