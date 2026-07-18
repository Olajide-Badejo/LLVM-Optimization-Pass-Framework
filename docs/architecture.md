# Architecture

The framework is one loadable module built from an object library shared with
the unit tests. Everything lives in namespace `opf` and layers cleanly from the
IR facts at the bottom to the tooling at the top.

## Layers

```
                     opt (stock binary)
                          |
           llvmGetPassPluginInfo  (plugin_registration.cpp)
                          |
        +-----------------+------------------+
        |                 |                  |
    analyses          transforms          support
  (read only)        (rewrite IR)       (shared glue)
        |                 |                  |
        +--------- LLVM IR / new PassManager +
```

### Analyses (`include/opf/analysis`, `lib/analysis`)

Each analysis is a new PassManager function analysis (`AnalysisInfoMixin`) that
returns a plain value struct: `OpcodeStats`, `BlockStats`, `DeadCodeReport`,
`SimplifyOpportunities`. The structs are pure data with `toJSON` and `print`
helpers, so the same numbers feed a human printer, the JSON emitter, and the
unit tests. Analyses never modify the IR and cache nothing themselves; the
analysis manager owns caching and invalidation.

The detection predicates the analyses use (is this instruction dead, is this an
algebraic identity, do these two instructions compute the same value) are free
functions, not private methods, precisely so the transforms can share them.

### Transforms (`include/opf/transforms`, `lib/transforms`)

Each transform is a free function core (`canonicalizeFunction`,
`algebraicSimplifyFunction`, `dceFunction`, `localCSEFunction`) returning a
rewrite count, wrapped by a thin `PassInfoMixin` pass. Splitting the core out
means the fixed point pipeline can call the functions directly and count
progress, and the transforms can be tested without a pass manager.

Every transform returns precise `PreservedAnalyses`: `all()` when it changed
nothing, otherwise `preserveSet<CFGAnalyses>()`, because these transforms touch
values and instructions but never the control flow graph. Over claiming here is
the classic bug, so a unit test exists specifically to catch it.

The `my-default` pipeline (`default_pipeline.cpp`) runs the four in the order
canonicalize, simplify, local CSE, DCE, repeating until an iteration makes no
change or the iteration cap is reached. The order matters: canonicalization
exposes duplicates for CSE, simplification creates dead code, DCE clears it, and
the cleanup can expose more for the next round.

### Support (`include/opf/support`, `lib/support`)

Cross cutting glue: `instruction_key` (the shared structural identity used by
both the duplicate count and local CSE), `pipeline_logging` (the per iteration
record), and `metrics_json` (the module pass that rolls every analysis into one
JSON document for the harness).

### Registration (`lib/plugin_registration.cpp`)

The single plugin entry point wires every analysis and pass into `PassBuilder`
through parsing and registration callbacks, so a stock `opt` picks them up by
name. This is the only file that knows the string names.

## Testing and tooling

The `opf_core` object library is linked into both the plugin module and a
GoogleTest binary, so unit tests exercise the exact code the plugin ships. lit
and FileCheck drive the plugin through `opt` for IR level tests. The Python
harness in `benchmarks/harness` compiles kernels, runs the pipeline, checks
differential execution, and writes CSV; `scripts/generate_plots.py` turns that
CSV into the report's figures and tables.
