# Design decisions

Short records of choices that a reader might otherwise question. Each is a
decision, its rationale, and its consequence.

## Distribution LLVM, no source build

The plugin links against a prebuilt apt LLVM (`llvm-N-dev`), never a source
build. Building LLVM from source would cost hours and tens of gigabytes and buy
nothing for out of tree middle end work. This is exactly how production teams
prototype passes.

## Pinned LLVM major, enforced in CMake

One cache variable, `OPF_LLVM_VERSION_MAJOR`, pins the major version (currently
21, the version apt ships on the target Ubuntu). Every script and CI job passes
the matching `llvm-config-N`, and `CMakeLists.txt` fails the configure if the
discovered LLVM disagrees. See the engineering log for why the pin is enforced
manually rather than through `find_package`'s version argument.

## New PassManager only

Passes use `PassInfoMixin`/`AnalysisInfoMixin` and register through
`llvmGetPassPluginInfo` with `PassBuilder` callbacks. The legacy pass manager is
deprecated for the middle end, and the new PM is what `opt -passes=...` drives
and what real LLVM development targets. The plugin therefore loads into a stock
`opt` with no patched binary.

## Naming conventions, deliberately split

Files use `snake_case`. Types use LLVM style `CamelCase`, variables `camelBack`,
and constants `UPPER_SNAKE`, matching the ecosystem the pass code lives inside.
Python is `snake_case` under `ruff`. Mixing conventions across the file boundary
is intentional: the C++ reads like LLVM because it is LLVM adjacent, while the
repository plumbing reads like an ordinary project.

## Quoted lit substitutions

Every lit RUN line quotes `%s`, `%opflib`, and tool paths so the suite passes
from any checkout location, including paths with spaces. See the engineering log
entry for the failure that motivated this.

## The my-default pipeline lives in its own file

The spec's file layout names the transforms and `support/pipeline_logging`, but
the fixed point driver that composes them needed a home. It lives in
`transforms/default_pipeline.{hpp,cpp}` as an extension of the transform layer,
with the per iteration record in `support/pipeline_logging`. The driver is a
free function so the integration test can inspect the iteration log directly.

## mem2reg before measuring

The benchmark harness runs `opt -passes=mem2reg` to form the SSA baseline before
either our pipeline or `opt -O1`. mem2reg is universal canonicalization into SSA
that any middle end assumes, not one of our optimizations, and running it for
both configurations keeps the comparison fair while giving the scalar transforms
registers to act on. This is documented in `docs/benchmarks.md` and recorded in
every run manifest.

## Figures and tables are generated, never committed by hand

`scripts/generate_plots.py` is the only thing that writes `report/figures` and
`report/tables`, straight from the CSV. The report `\input`s the generated
files, so there is no path by which a hand typed number reaches the PDF.

## Analysis and transform separation

Analyses compute and cache facts and declare invalidation correctly; transforms
consume them and return precise `PreservedAnalyses`. A dedicated test proves a
mutated IR forces a dependent analysis to recompute rather than serving a stale
cached result.
