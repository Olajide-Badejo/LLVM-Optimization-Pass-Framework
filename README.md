# llvm-optimization-pass-framework

Out of tree LLVM new pass manager framework: custom analysis and transformation
passes with lit/FileCheck tests, differential execution validation, and a
measured before/after benchmark report.

The framework builds as a single loadable module that a stock `opt` picks up
with `-load-pass-plugin`. It provides analysis passes that report real IR
metrics, a set of conservative transformation passes, and a fixed point pipeline
that composes them, all on the new PassManager.

## Status

Under construction, built phase by phase against the roadmap in the build
specification. The pinned toolchain is **LLVM 21** (apt `llvm-21` on Ubuntu).
Runtime numbers in this README are placeholders until the benchmark suite has
run on the target machine; they will be replaced with measured wall clock.

## Requirements

Linux, native or WSL2. On the target machine the project is built inside WSL2
Ubuntu against distribution LLVM. No LLVM source build is required or performed.

```
sudo apt-get install llvm-21-dev clang-21 llvm-21-tools clang-format-21 \
                     cmake ninja-build latexmk texlive-latex-extra python3-venv
```

If a different pinned major is installed, pass `OPF_LLVM_MAJOR=<N>` to the make
targets and scripts.

## Build

```
make build
```

This configures with CMake and Ninja against the LLVM that `llvm-config-21`
reports, and produces `build/lib/libOPFPasses.so`.

## Test

```
make test         # build, then unit (ctest) and lit/FileCheck suites
make check-style  # dash lint and clang-format check
```

## Run a pass by hand

```
opt -load-pass-plugin=build/lib/libOPFPasses.so \
    -passes="my-noop" -S input.ll
```

Once later phases land, the same mechanism drives the real pipeline, for example
`-passes="my-default"` or `-passes="my-canon,my-simplify,my-dce"`.

## Layout

```
include/opf/   public headers: analysis/, transforms/, support/
lib/           one .cpp per header, plus plugin_registration.cpp
tests/         lit/ (FileCheck IR tests), unit/ (GoogleTest)
benchmarks/    kernels/, harness/, results/
scripts/       build, test, benchmark, plot, report helpers
docs/          architecture, passes, benchmarks, usage, decisions, log
report/        main.tex and generated figures/tables
report_debug/  postmortem debug report
```

## License

Apache License 2.0 with LLVM Exceptions. See `LICENSE`.
