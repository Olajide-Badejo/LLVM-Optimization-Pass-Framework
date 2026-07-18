//===- metrics_json.hpp - one JSON document of all IR metrics -------------===//
//
// Part of the llvm-optimization-pass-framework project.
// Licensed under the MIT License.
//
//===----------------------------------------------------------------------===//
//
// A module pass that runs every analysis the framework provides over each
// function and emits a single JSON document: per function metrics plus module
// wide totals. The benchmark harness runs one opt invocation with this pass and
// parses the result, rather than scraping several human printers.
//
//===----------------------------------------------------------------------===//

#ifndef OPF_SUPPORT_METRICS_JSON_HPP
#define OPF_SUPPORT_METRICS_JSON_HPP

#include "llvm/IR/PassManager.h"

namespace llvm {
class Module;
class raw_ostream;
} // namespace llvm

namespace opf {

// Registered under the pipeline name "my-metrics-json".
class MetricsJSONEmitter : public llvm::PassInfoMixin<MetricsJSONEmitter> {
  llvm::raw_ostream &OS;

public:
  explicit MetricsJSONEmitter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }
};

} // namespace opf

#endif // OPF_SUPPORT_METRICS_JSON_HPP
