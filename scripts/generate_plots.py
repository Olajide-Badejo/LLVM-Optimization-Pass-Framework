#!/usr/bin/env python3
"""Turn the benchmark CSVs into report figures and LaTeX tables.

Every figure and table the report shows is generated here from the committed
CSVs, so the report contains zero hand typed numbers. Running this twice
produces byte identical output (idempotent). Colors come from the Okabe and Ito
palette, a colorblind safe categorical set, assigned to the three configurations
in a fixed order so identity never depends on rank.
"""

from __future__ import annotations

import csv
import sys
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from tqdm import tqdm
except ImportError as exc:  # pragma: no cover
    print("error: matplotlib and tqdm are required. Run through "
          "scripts/run_benchmarks.sh once to create the virtualenv, then use "
          "that interpreter.", file=sys.stderr)
    raise SystemExit(2) from exc

REPO_ROOT = Path(__file__).resolve().parent.parent
RESULTS = REPO_ROOT / "benchmarks" / "results"
BENCH_CSV = RESULTS / "benchmarks.csv"
OPCODES_CSV = RESULTS / "opcodes.csv"
FIG_DIR = REPO_ROOT / "report" / "figures"
TAB_DIR = REPO_ROOT / "report" / "tables"
# PNG copies of the figures for the README, which GitHub renders inline (it does
# not render PDF inline).
ASSETS_IMG = REPO_ROOT / "assets" / "images"

CONFIGS = ["baseline", "ours", "o1"]
CONFIG_LABELS = {"baseline": "baseline", "ours": "my-default", "o1": "opt -O1"}

# Okabe and Ito, fixed order per configuration. Colorblind safe and well
# separated for full color vision too.
CONFIG_COLORS = {
    "baseline": "#0072B2",  # blue
    "ours": "#D55E00",      # vermillion
    "o1": "#009E73",        # bluish green
}

# The kernel used to illustrate the opcode level change in detail.
SHOWCASE_KERNEL = "arith_redundant"

INK = "#222222"
MUTED = "#666666"
GRID = "#DDDDDD"


def configure_style() -> None:
    plt.rcParams.update({
        "figure.dpi": 120,
        "savefig.bbox": "tight",
        "font.size": 10,
        "axes.edgecolor": MUTED,
        "axes.labelcolor": INK,
        "axes.titlecolor": INK,
        "xtick.color": INK,
        "ytick.color": INK,
        "axes.spines.top": False,
        "axes.spines.right": False,
        "axes.grid": True,
        "axes.axisbelow": True,
        "grid.color": GRID,
        "grid.linewidth": 0.8,
    })


def save_figure(fig, stem: str) -> None:
    """Save a figure as a PDF for the report and a PNG for the README."""
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    ASSETS_IMG.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIG_DIR / f"{stem}.pdf")
    fig.savefig(ASSETS_IMG / f"{stem}.png", dpi=150)


def read_bench() -> list[dict]:
    with open(BENCH_CSV, newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def read_opcodes() -> list[dict]:
    with open(OPCODES_CSV, newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def kernels_in(rows: list[dict]) -> list[str]:
    seen: list[str] = []
    for row in rows:
        if row["kernel"] not in seen:
            seen.append(row["kernel"])
    return sorted(seen)


def by_kernel_config(rows: list[dict], field: str) -> dict:
    table: dict[tuple[str, str], float] = {}
    for row in rows:
        table[(row["kernel"], row["config"])] = float(row[field])
    return table


def grouped_bars(ax, kernels, series, values, errors=None, labels=True):
    """Draw grouped bars: one group per kernel, one bar per config.

    Labels are drawn only when they are clean integers (instruction counts).
    Runtime bars carry error bars instead, and their exact values live in the
    runtime table, so labelling every bar there would only add clutter.
    """
    n = len(series)
    width = 0.8 / n
    positions = range(len(kernels))
    for i, cfg in enumerate(series):
        offsets = [p - 0.4 + width * (i + 0.5) for p in positions]
        heights = [values[(k, cfg)] for k in kernels]
        err = None
        if errors is not None:
            err = [errors[(k, cfg)] for k in kernels]
        bars = ax.bar(offsets, heights, width * 0.9, label=CONFIG_LABELS[cfg],
                      color=CONFIG_COLORS[cfg], yerr=err, capsize=3,
                      error_kw={"ecolor": MUTED, "elinewidth": 1})
        if labels:
            for rect, height in zip(bars, heights, strict=True):
                ax.annotate(f"{height:g}",
                            xy=(rect.get_x() + rect.get_width() / 2, height),
                            xytext=(0, 2), textcoords="offset points",
                            ha="center", va="bottom", fontsize=7, color=MUTED)
    ax.set_xticks(list(positions))
    ax.set_xticklabels(kernels, rotation=0)
    ax.legend(frameon=False, fontsize=9)


def fig_instruction_counts(rows: list[dict]) -> None:
    kernels = kernels_in(rows)
    values = by_kernel_config(rows, "total_instructions")
    fig, ax = plt.subplots(figsize=(7, 4))
    grouped_bars(ax, kernels, CONFIGS, values)
    ax.set_ylabel("total IR instructions")
    ax.set_title("IR instruction count by kernel and configuration")
    save_figure(fig, "instruction_counts")
    plt.close(fig)


def fig_instruction_reduction(rows: list[dict]) -> None:
    kernels = kernels_in(rows)
    counts = by_kernel_config(rows, "total_instructions")
    reductions = []
    for k in kernels:
        base = counts[(k, "baseline")]
        ours = counts[(k, "ours")]
        pct = 100.0 * (base - ours) / base if base else 0.0
        reductions.append(pct)
    fig, ax = plt.subplots(figsize=(7, 4))
    bars = ax.bar(kernels, reductions, color=CONFIG_COLORS["ours"], width=0.6)
    for rect, pct in zip(bars, reductions, strict=True):
        ax.annotate(f"{pct:.1f}%",
                    xy=(rect.get_x() + rect.get_width() / 2, pct),
                    xytext=(0, 2), textcoords="offset points",
                    ha="center", va="bottom", fontsize=8, color=MUTED)
    ax.set_ylabel("instruction reduction by my-default (percent)")
    ax.set_title("How much the framework removes, relative to the SSA baseline")
    save_figure(fig, "instruction_reduction")
    plt.close(fig)


def fig_runtime(rows: list[dict]) -> None:
    kernels = kernels_in(rows)
    medians = by_kernel_config(rows, "runtime_ms_median")
    iqrs = by_kernel_config(rows, "runtime_ms_iqr")
    fig, ax = plt.subplots(figsize=(7, 4))
    grouped_bars(ax, kernels, CONFIGS, medians, errors=iqrs, labels=False)
    ax.set_ylabel("median runtime (ms), error bar is IQR")
    ax.set_title("Runtime by kernel and configuration (backend at -O0)")
    save_figure(fig, "runtime_medians")
    plt.close(fig)


def fig_opcode_delta(opcodes: list[dict]) -> None:
    rows = [r for r in opcodes if r["kernel"] == SHOWCASE_KERNEL
            and r["config"] in ("baseline", "ours")]
    if not rows:
        return
    ops = sorted({r["opcode"] for r in rows})
    lookup = {(r["opcode"], r["config"]): int(r["count"]) for r in rows}
    base_counts = [lookup.get((op, "baseline"), 0) for op in ops]
    our_counts = [lookup.get((op, "ours"), 0) for op in ops]

    fig, ax = plt.subplots(figsize=(7, 4))
    width = 0.38
    positions = range(len(ops))
    ax.bar([p - width / 2 for p in positions], base_counts, width,
           label=CONFIG_LABELS["baseline"], color=CONFIG_COLORS["baseline"])
    ax.bar([p + width / 2 for p in positions], our_counts, width,
           label=CONFIG_LABELS["ours"], color=CONFIG_COLORS["ours"])
    ax.set_xticks(list(positions))
    ax.set_xticklabels(ops, rotation=45, ha="right")
    ax.set_ylabel("count")
    ax.set_title(f"Per opcode counts, {SHOWCASE_KERNEL}: baseline vs my-default")
    ax.legend(frameon=False, fontsize=9)
    save_figure(fig, "opcode_delta")
    plt.close(fig)


def latex_escape(text: str) -> str:
    return text.replace("_", r"\_")


def table_structural(rows: list[dict]) -> None:
    kernels = kernels_in(rows)
    counts = by_kernel_config(rows, "total_instructions")
    lines = [
        r"\begin{tabular}{lrrrr}",
        r"\toprule",
        r"kernel & baseline & my-default & reduction & opt -O1 \\",
        r"\midrule",
    ]
    for k in kernels:
        base = int(counts[(k, "baseline")])
        ours = int(counts[(k, "ours")])
        o1 = int(counts[(k, "o1")])
        pct = 100.0 * (base - ours) / base if base else 0.0
        lines.append(
            f"{latex_escape(k)} & {base} & {ours} & {pct:.1f}\\% & {o1} \\\\")
    lines += [r"\bottomrule", r"\end{tabular}"]
    (TAB_DIR / "table_structural.tex").write_text(
        "\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def table_runtime(rows: list[dict]) -> None:
    kernels = kernels_in(rows)
    medians = by_kernel_config(rows, "runtime_ms_median")
    iqrs = by_kernel_config(rows, "runtime_ms_iqr")
    lines = [
        r"\begin{tabular}{llrr}",
        r"\toprule",
        r"kernel & configuration & median (ms) & IQR (ms) \\",
        r"\midrule",
    ]
    for k in kernels:
        for cfg in CONFIGS:
            lines.append(
                f"{latex_escape(k)} & {CONFIG_LABELS[cfg]} & "
                f"{medians[(k, cfg)]:.2f} & {iqrs[(k, cfg)]:.2f} \\\\")
        lines.append(r"\addlinespace")
    lines += [r"\bottomrule", r"\end{tabular}"]
    (TAB_DIR / "table_runtime.tex").write_text(
        "\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def table_checksum(rows: list[dict]) -> None:
    kernels = kernels_in(rows)
    checks: dict[str, set] = {}
    sample: dict[str, str] = {}
    for row in rows:
        checks.setdefault(row["kernel"], set()).add(row["checksum"])
        sample[row["kernel"]] = row["checksum"]
    lines = [
        r"\begin{tabular}{lll}",
        r"\toprule",
        r"kernel & checksum & configurations agree \\",
        r"\midrule",
    ]
    for k in kernels:
        agree = "yes" if len(checks[k]) == 1 else "NO"
        lines.append(
            f"{latex_escape(k)} & \\texttt{{{sample[k]}}} & {agree} \\\\")
    lines += [r"\bottomrule", r"\end{tabular}"]
    (TAB_DIR / "table_checksum.tex").write_text(
        "\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def camel(name: str) -> str:
    return "".join(part.capitalize() for part in name.replace("-", "_").split("_"))


def table_macros(rows: list[dict]) -> None:
    """Emit LaTeX macros for every headline number the prose refers to.

    This is what lets the report body cite concrete values without a human ever
    typing one: the prose says \\ArithRedundantReduction, and that expands to a
    number computed here from the CSV.
    """
    kernels = kernels_in(rows)
    instrs = by_kernel_config(rows, "total_instructions")
    medians = by_kernel_config(rows, "runtime_ms_median")
    lines = [
        "% Generated by generate_plots.py. Do not edit.",
        f"\\newcommand{{\\NumKernels}}{{{len(kernels)}}}",
    ]
    for k in kernels:
        c = camel(k)
        base = int(instrs[(k, "baseline")])
        ours = int(instrs[(k, "ours")])
        o1 = int(instrs[(k, "o1")])
        pct = 100.0 * (base - ours) / base if base else 0.0
        lines += [
            f"\\newcommand{{\\{c}Base}}{{{base}}}",
            f"\\newcommand{{\\{c}Ours}}{{{ours}}}",
            f"\\newcommand{{\\{c}OOne}}{{{o1}}}",
            f"\\newcommand{{\\{c}Reduction}}{{{pct:.1f}}}",
            f"\\newcommand{{\\{c}BaseMs}}{{{medians[(k, 'baseline')]:.1f}}}",
            f"\\newcommand{{\\{c}OursMs}}{{{medians[(k, 'ours')]:.1f}}}",
            f"\\newcommand{{\\{c}OOneMs}}{{{medians[(k, 'o1')]:.1f}}}",
        ]
    (TAB_DIR / "values.tex").write_text(
        "\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    if not BENCH_CSV.exists():
        print(f"error: {BENCH_CSV} not found. Run the benchmarks first with "
              "scripts/run_benchmarks.sh.", file=sys.stderr)
        return 2

    configure_style()
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    TAB_DIR.mkdir(parents=True, exist_ok=True)

    bench = read_bench()
    opcodes = read_opcodes()

    tasks = [
        ("figure: instruction counts", lambda: fig_instruction_counts(bench)),
        ("figure: instruction reduction",
         lambda: fig_instruction_reduction(bench)),
        ("figure: runtime medians", lambda: fig_runtime(bench)),
        ("figure: opcode delta", lambda: fig_opcode_delta(opcodes)),
        ("table: structural", lambda: table_structural(bench)),
        ("table: runtime", lambda: table_runtime(bench)),
        ("table: checksum", lambda: table_checksum(bench)),
        ("table: value macros", lambda: table_macros(bench)),
    ]
    for _, fn in tqdm(tasks, desc="figures and tables", unit="artifact"):
        fn()

    print(f"wrote figures to {FIG_DIR.relative_to(REPO_ROOT)}")
    print(f"wrote tables to {TAB_DIR.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
