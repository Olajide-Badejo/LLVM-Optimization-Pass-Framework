//===- pipeline_logging.hpp - per iteration pipeline record --------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//
//
// A small record of what the fixed point pipeline did: for each iteration,
// which pass fired and how many rewrites it made, and whether the run reached a
// fixed point or stopped at the iteration cap. The docs and the report reuse
// this summary, and the integration test asserts on it directly.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_SUPPORT_PIPELINE_LOGGING_HPP
#define OPF_SUPPORT_PIPELINE_LOGGING_HPP

#include <string>
#include <vector>

namespace llvm {
class raw_ostream;
class StringRef;
} // namespace llvm

namespace opf {

struct PassRun {
  std::string Pass;
  unsigned Rewrites = 0;
};

struct IterationLog {
  unsigned Index = 0;
  std::vector<PassRun> Runs;

  unsigned total() const;
};

class PipelineLog {
public:
  void setSubject(llvm::StringRef Name);

  // Starts a new iteration and returns a reference to fill in.
  IterationLog &newIteration();

  void markReachedCap(bool Hit) { HitCap = Hit; }

  unsigned numIterations() const {
    return static_cast<unsigned>(Iterations.size());
  }
  unsigned totalRewrites() const;
  bool reachedFixedPoint() const { return !HitCap; }
  const std::vector<IterationLog> &iterations() const { return Iterations; }

  void print(llvm::raw_ostream &OS) const;

private:
  std::string Subject;
  std::vector<IterationLog> Iterations;
  bool HitCap = false;
};

} // namespace opf

#endif // OPF_SUPPORT_PIPELINE_LOGGING_HPP
