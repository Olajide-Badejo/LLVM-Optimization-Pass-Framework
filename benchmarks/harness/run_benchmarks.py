#!/usr/bin/env python3
"""Benchmark harness for the LLVM optimization pass framework.

For every kernel this script runs one deterministic pipeline:

    clang -O0 (optnone disabled)  ->  raw IR
    opt mem2reg                   ->  baseline IR in SSA form
    opt my-default (the plugin)   ->  our transformed IR
    opt -O1                       ->  reference IR

It records the structural metrics of each configuration (via the plugin's
my-metrics-json emitter), compiles each to a binary, runs each repeatedly under
taskset with warmup discards, and checks that the baseline, our, and O1 builds
all print the same checksum. A mismatch aborts the run, because a transform that
changes observable output is a bug no matter what the instruction counts say.

Every number here comes from a real run on real hardware; nothing is invented.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

try:
    from tqdm import tqdm
except ImportError as exc:  # pragma: no cover
    print("error: tqdm is required. Run scripts/run_benchmarks.sh, which sets "
          "up a virtualenv with it.", file=sys.stderr)
    raise SystemExit(2) from exc

REPO_ROOT = Path(__file__).resolve().parents[2]
KERNELS_DIR = REPO_ROOT / "benchmarks" / "kernels"
RESULTS_DIR = REPO_ROOT / "benchmarks" / "results"
RAW_DIR = RESULTS_DIR / "raw"
CANONICAL_CSV = RESULTS_DIR / "benchmarks.csv"
CANONICAL_OPCODES_CSV = RESULTS_DIR / "opcodes.csv"

LLVM_MAJOR = os.environ.get("OPF_LLVM_MAJOR", "21")
CLANG = f"clang-{LLVM_MAJOR}"
OPT = f"opt-{LLVM_MAJOR}"
LLVM_CONFIG = f"llvm-config-{LLVM_MAJOR}"
PLUGIN = REPO_ROOT / "build" / "lib" / "libOPFPasses.so"

# The configurations compared per kernel. "baseline" is the SSA form before any
# of our passes; "ours" is the my-default pipeline; "o1" is stock opt -O1.
CONFIGS = ["baseline", "ours", "o1"]

CSV_FIELDS = [
    "kernel",
    "config",
    "total_instructions",
    "total_blocks",
    "total_dead_instructions",
    "total_simplify_opportunities",
    "runtime_ms_median",
    "runtime_ms_min",
    "runtime_ms_max",
    "runtime_ms_iqr",
    "repetitions",
    "warmup",
    "checksum",
]


def run(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    """Run a command as an argument list so paths with spaces are safe."""
    return subprocess.run(
        cmd, check=True, capture_output=True, text=True, **kwargs
    )


def tool_version(tool: str) -> str:
    try:
        out = run([tool, "--version"]).stdout
        return out.strip().splitlines()[0]
    except Exception:  # pragma: no cover
        return "unknown"


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=str(path.parent), suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)


def compile_configs(kernel: Path, work: Path) -> dict[str, Path]:
    """Produce the baseline, ours, and o1 IR files for one kernel."""
    raw = work / "raw.ll"
    base = work / "baseline.ll"
    ours = work / "ours.ll"
    o1 = work / "o1.ll"

    # optnone would make opt skip these functions entirely, so disable it.
    run([CLANG, "-O0", "-Xclang", "-disable-O0-optnone", "-emit-llvm", "-S",
         str(kernel), "-o", str(raw)])
    # mem2reg is universal canonicalization into SSA, not one of our passes; it
    # gives the scalar transforms registers to work on. Both ours and o1 start
    # from this same baseline so the comparison is fair.
    run([OPT, "-passes=mem2reg", str(raw), "-S", "-o", str(base)])
    run([OPT, "-load-pass-plugin", str(PLUGIN), "-passes=my-default",
         str(base), "-S", "-o", str(ours)])
    run([OPT, "-O1", str(base), "-S", "-o", str(o1)])
    return {"baseline": base, "ours": ours, "o1": o1}


def structural_metrics(ir: Path) -> dict:
    """Run the plugin's JSON emitter and return the module totals."""
    proc = run([OPT, "-load-pass-plugin", str(PLUGIN),
                "-passes=my-metrics-json", "-disable-output", str(ir)])
    data = json.loads(proc.stdout)
    return data["totals"]


def build_binary(ir: Path, out: Path) -> None:
    # Compile at -O0 so the backend does not re-optimize and mask the middle end
    # differences we are measuring.
    run([CLANG, "-O0", str(ir), "-o", str(out)])


def time_binary(binary: Path, reps: int, warmup: int,
                cores: str | None) -> tuple[list[float], str]:
    """Run the binary reps+warmup times; return kept timings (ms) and checksum."""
    prefix = ["taskset", "-c", cores] if cores else []
    checksum = None
    timings: list[float] = []
    for i in range(reps + warmup):
        start = time.perf_counter()
        proc = run(prefix + [str(binary)])
        elapsed_ms = (time.perf_counter() - start) * 1000.0
        out = proc.stdout.strip()
        if checksum is None:
            checksum = out
        elif out != checksum:  # pragma: no cover
            raise SystemExit(
                f"nondeterministic checksum from {binary}: {out} != {checksum}")
        if i >= warmup:
            timings.append(elapsed_ms)
    return timings, checksum or ""


def iqr(values: list[float]) -> float:
    if len(values) < 2:
        return 0.0
    ordered = sorted(values)
    mid = len(ordered) // 2
    lower = statistics.median(ordered[:mid])
    upper = statistics.median(ordered[-mid:])
    return upper - lower


def process_kernel(kernel: Path, reps: int, warmup: int,
                   cores: str | None) -> tuple[list[dict], list[dict]]:
    name = kernel.stem
    with tempfile.TemporaryDirectory(prefix=f"opf_{name}_") as tmp:
        work = Path(tmp)
        irs = compile_configs(kernel, work)

        metrics = {cfg: structural_metrics(irs[cfg]) for cfg in CONFIGS}

        checksums: dict[str, str] = {}
        rows: list[dict] = []
        opcode_rows: list[dict] = []
        for cfg in CONFIGS:
            binary = work / f"{cfg}.bin"
            build_binary(irs[cfg], binary)
            timings, checksum = time_binary(binary, reps, warmup, cores)
            checksums[cfg] = checksum

            rows.append({
                "kernel": name,
                "config": cfg,
                "total_instructions": metrics[cfg]["total_instructions"],
                "total_blocks": metrics[cfg]["total_blocks"],
                "total_dead_instructions":
                    metrics[cfg]["total_dead_instructions"],
                "total_simplify_opportunities":
                    metrics[cfg]["total_simplify_opportunities"],
                "runtime_ms_median": round(statistics.median(timings), 4),
                "runtime_ms_min": round(min(timings), 4),
                "runtime_ms_max": round(max(timings), 4),
                "runtime_ms_iqr": round(iqr(timings), 4),
                "repetitions": reps,
                "warmup": warmup,
                "checksum": checksum,
            })
            for opcode, count in metrics[cfg]["by_opcode"].items():
                opcode_rows.append({
                    "kernel": name,
                    "config": cfg,
                    "opcode": opcode,
                    "count": count,
                })

        # The gate: every configuration must compute the same result.
        distinct = set(checksums.values())
        if len(distinct) != 1:
            raise SystemExit(
                f"CHECKSUM MISMATCH on kernel '{name}': {checksums}. "
                "A transform changed observable output; aborting.")

        return rows, opcode_rows


def load_existing(csv_path: Path, key_fields: tuple[str, ...]) -> dict:
    if not csv_path.exists():
        return {}
    with open(csv_path, newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        return {tuple(row[k] for k in key_fields): row for row in reader}


def write_csv(path: Path, fields: list[str], rows: list[dict]) -> None:
    buffer = []
    buffer.append(",".join(fields))
    for row in rows:
        buffer.append(",".join(str(row[f]) for f in fields))
    atomic_write_text(path, "\n".join(buffer) + "\n")


def cpu_governor_note() -> str:
    gov = Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
    if gov.exists():
        try:
            return gov.read_text().strip()
        except OSError:
            pass
    return "unavailable (typical under WSL2; timings are best effort)"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reps", type=int, default=7,
                        help="timed repetitions per binary (default 7)")
    parser.add_argument("--warmup", type=int, default=2,
                        help="warmup runs discarded before timing (default 2)")
    parser.add_argument("--cores", default="0-7",
                        help="taskset core list to pin to, or 'none'")
    parser.add_argument("--force", action="store_true",
                        help="recompute kernels already present in the CSV")
    parser.add_argument("--kernel", action="append", default=None,
                        help="limit to named kernel(s); repeatable")
    args = parser.parse_args()

    if not PLUGIN.exists():
        print(f"error: plugin not found at {PLUGIN}. Build first with "
              "'make build'.", file=sys.stderr)
        return 2

    cores = None if args.cores == "none" else args.cores
    if cores and shutil.which("taskset") is None:
        print("note: taskset not found; running without core pinning.",
              file=sys.stderr)
        cores = None

    kernels = sorted(KERNELS_DIR.glob("*.c"))
    if args.kernel:
        wanted = set(args.kernel)
        kernels = [k for k in kernels if k.stem in wanted]
    if not kernels:
        print("error: no kernels found.", file=sys.stderr)
        return 2

    # Resumability: keep rows for kernels already measured unless forced.
    existing_bench = load_existing(CANONICAL_CSV, ("kernel", "config"))
    existing_opcode = load_existing(
        CANONICAL_OPCODES_CSV, ("kernel", "config", "opcode"))
    done_kernels = {key[0] for key in existing_bench}

    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    try:
        sha = run(["git", "-C", str(REPO_ROOT), "rev-parse", "--short",
                   "HEAD"]).stdout.strip()
    except Exception:
        sha = "nogit"
    run_dir = RAW_DIR / f"{timestamp}_{sha}"
    run_dir.mkdir(parents=True, exist_ok=True)

    planned = [k for k in kernels
               if args.force or k.stem not in done_kernels]
    print(f"kernels: {len(kernels)} total, {len(planned)} to run "
          f"(configs: {', '.join(CONFIGS)}, reps: {args.reps}, "
          f"warmup: {args.warmup}, cores: {cores or 'none'})")
    projected = len(planned) * len(CONFIGS) * (args.reps + args.warmup)
    print(f"projected binary executions: {projected}")

    bench_rows: list[dict] = list(existing_bench.values())
    opcode_rows: list[dict] = list(existing_opcode.values())

    for kernel in tqdm(kernels, desc="kernels", unit="kernel"):
        if not args.force and kernel.stem in done_kernels:
            continue
        new_bench, new_opcode = process_kernel(
            kernel, args.reps, args.warmup, cores)
        # Drop any stale rows for this kernel, then add the fresh ones.
        bench_rows = [r for r in bench_rows if r["kernel"] != kernel.stem]
        opcode_rows = [r for r in opcode_rows if r["kernel"] != kernel.stem]
        bench_rows.extend(new_bench)
        opcode_rows.extend(new_opcode)

    # Stable ordering for a clean diff on the committed CSV.
    bench_rows.sort(key=lambda r: (r["kernel"], CONFIGS.index(r["config"])))
    opcode_rows.sort(
        key=lambda r: (r["kernel"], CONFIGS.index(r["config"]), r["opcode"]))

    manifest = {
        "timestamp_utc": timestamp,
        "git_sha": sha,
        "llvm_version": tool_version(LLVM_CONFIG),
        "clang_version": tool_version(CLANG),
        "opt_version": tool_version(OPT),
        "host": platform.platform(),
        "python": platform.python_version(),
        "kernels": [k.stem for k in kernels],
        "configs": CONFIGS,
        "repetitions": args.reps,
        "warmup": args.warmup,
        "taskset_cores": cores or "none",
        "cpu_governor": cpu_governor_note(),
        "clang_flags": "-O0 -Xclang -disable-O0-optnone",
        "backend_flags_for_timing": "-O0",
        "notes": "mem2reg applied before our pipeline and before -O1 so all "
                 "configurations start from the same SSA baseline.",
    }

    # Write the timestamped raw record and update the canonical committed CSVs.
    atomic_write_text(run_dir / "manifest.json",
                      json.dumps(manifest, indent=2) + "\n")
    write_csv(run_dir / "benchmarks.csv", CSV_FIELDS, bench_rows)
    write_csv(run_dir / "opcodes.csv",
              ["kernel", "config", "opcode", "count"], opcode_rows)
    write_csv(CANONICAL_CSV, CSV_FIELDS, bench_rows)
    write_csv(CANONICAL_OPCODES_CSV,
              ["kernel", "config", "opcode", "count"], opcode_rows)

    print(f"\nwrote {CANONICAL_CSV.relative_to(REPO_ROOT)} "
          f"({len(bench_rows)} rows)")
    print(f"wrote {CANONICAL_OPCODES_CSV.relative_to(REPO_ROOT)} "
          f"({len(opcode_rows)} rows)")
    print(f"raw record: {run_dir.relative_to(REPO_ROOT)}")
    print("all checksums matched across baseline, ours, and o1.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
