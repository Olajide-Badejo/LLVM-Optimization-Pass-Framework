# llvm-optimization-pass-framework

Out of tree LLVM new pass manager framework: custom analysis and transformation
passes with lit/FileCheck tests, differential execution validation, and a
measured before/after benchmark report.

The framework builds as a single loadable module that a stock `opt` picks up
with `-load-pass-plugin`. It provides analysis passes that report real IR
metrics, a set of conservative transformation passes, and a fixed point pipeline
that composes them, all on the new PassManager.

## Status

Complete and tagged `v1.0.0`. The pinned toolchain is **LLVM 21** (apt
`llvm-21` on Ubuntu). All unit, lit, integration, and differential tests pass,
the benchmark suite has run on the target machine, and both report PDFs build
with zero errors.

### Measured wall clock

On the target machine (Intel Core i7-14700K, 32 GB, WSL2 Ubuntu), timed with
`/usr/bin/time`:

- Clean build plus full test suite (unit and lit): about 1 minute (measured 59
  seconds). No LLVM source build; links against distribution LLVM.
- Full benchmark suite (all kernels, baseline vs framework vs `opt -O1`, timed
  with warmup and repetitions): about 7 seconds (measured 6.6 seconds).
- Each report PDF: a few seconds.

These replace the pre build estimates from the specification.

## Requirements

Linux, native or WSL2. On the target machine the project is built inside WSL2
Ubuntu against distribution LLVM. No LLVM source build is required or performed.

```bash
sudo apt-get install llvm-21-dev clang-21 llvm-21-tools clang-format-21 \
                     cmake ninja-build latexmk texlive-latex-extra python3-venv
```

If a different pinned major is installed, pass `OPF_LLVM_MAJOR=<N>` to the make
targets and scripts.

## Build

```bash
make build
```

This configures with CMake and Ninja against the LLVM that `llvm-config-21`
reports, and produces `build/lib/libOPFPasses.so`.

## Test

```bash
make test         # build, then unit (ctest) and lit/FileCheck suites
make check-style  # dash lint, clang-format, and ruff
```

## Run a pass by hand

```bash
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
       -passes="my-default" -S input.ll
```

The same mechanism runs any registered pass or pipeline, for example the
individual stages `-passes="my-canon,my-simplify,my-local-cse,my-dce"` or an
analysis printer such as `-passes="my-print-opcode-stats"`. See
[docs/usage.md](docs/usage.md) for the full list.

## Benchmarks and report

```bash
make bench          # run the differential benchmark suite, write the CSV
make report         # build report/main.pdf from the committed CSV
make report-debug   # build report_debug/debug_report.pdf
```

`scripts/generate_plots.py` turns the benchmark CSV into the report figures and
tables, so no number in the report is typed by hand. See
[docs/benchmarks.md](docs/benchmarks.md).

## Layout

```text
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
