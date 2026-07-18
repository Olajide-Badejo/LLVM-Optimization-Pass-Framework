# Benchmarks

## What the harness does

For each kernel in `benchmarks/kernels`, `run_benchmarks.py` runs one
deterministic pipeline:

1. `clang -O0 -Xclang -disable-O0-optnone -emit-llvm` produces raw IR. Disabling
   optnone is essential; otherwise `opt` skips the functions.
2. `opt -passes=mem2reg` promotes the stack locals to registers, giving an SSA
   baseline. mem2reg is universal canonicalization into SSA, not one of our
   passes; both our pipeline and `opt -O1` start from this same baseline so the
   comparison is fair and the scalar transforms have registers to work on.
3. `opt -passes=my-default` (the plugin) produces our transformed IR.
4. `opt -O1` produces the reference IR.
5. Structural metrics for all three come from the plugin's `my-metrics-json`.
6. Each configuration is compiled at `-O0` (so the backend does not re-optimize
   and hide the middle end differences) and run repeatedly under `taskset` with
   warmup discards; the median runtime and interquartile range are recorded.
7. Every kernel main prints a checksum. The run aborts unless the baseline, our,
   and O1 binaries all print the same checksum. A transform that changes
   observable output is a bug no matter how much IR it removed.

Output goes to a timestamped directory under `benchmarks/results/raw` (with a
`manifest.json` recording versions, flags, repetition counts, and the taskset
core list) and to the committed canonical `benchmarks.csv` and `opcodes.csv`.
Runs are resumable: a kernel already present in the CSV is skipped unless
`--force` is given.

## Running

```
make bench                        # all kernels
scripts/run_benchmarks.sh --kernel arith_redundant   # just one
scripts/run_benchmarks.sh --force                    # redo everything
```

The first invocation creates a virtualenv under `benchmarks/harness/.venv` with
`tqdm`, `matplotlib`, and `ruff`.

## Figures and tables

`scripts/generate_plots.py` reads the two CSVs and writes the report figures to
`report/figures` and LaTeX tables to `report/tables`. It is idempotent, so the
report never contains a hand typed number.

## Reading the results honestly

The kernels are chosen to show the framework's reach and its limits:

- `arith_redundant` has genuine scalar redundancy and identities, so our passes
  both shrink the IR and speed the kernel up measurably.
- `dead_code` is where `opt -O1` pulls far ahead: it recognizes the whole
  computation is dead and removes the loop, which our block local passes do not.
- `branchy` is simplified only inside its blocks; we deliberately do not fold
  branches or merge blocks, so the structural change is small and the runtime
  change is within noise.
- `clean` is already optimal, and the framework correctly makes no change. That
  is reported as an honest negative, not hidden.

## Adding a kernel

1. Drop a `.c` file in `benchmarks/kernels`. It must have a `main` that does a
   sized amount of work and prints a single deterministic checksum on stdout.
2. Run `scripts/run_benchmarks.sh --kernel <name>`. The differential check will
   tell you immediately if a transform miscompiles it.
3. Regenerate the figures with `scripts/generate_plots.py`.
