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

## Analysis and transform separation

Analyses compute and cache facts and declare invalidation correctly; transforms
consume them and return precise `PreservedAnalyses`. A dedicated test proves a
mutated IR forces a dependent analysis to recompute rather than serving a stale
cached result.
