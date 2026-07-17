#!/usr/bin/env python3
"""Style gate: forbid em dashes and en dashes across the repository.

Ground rule from the build specification: no U+2014 (em dash) and no U+2013 (en
dash) in any file, and no "--" or "---" runs in LaTeX prose (where they render
as those same dashes). This script enforces both and is wired into
`make check-style` and CI, so a stray dash fails the build the moment it lands.

The dash characters are referenced by escape here on purpose, so the linter
never trips over its own source.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

EM_DASH = chr(0x2014)
EN_DASH = chr(0x2013)

# Directories that never contain hand written prose we control.
SKIP_DIRS = {
    ".git",
    "build",
    "__pycache__",
    ".venv",
    "figures",
    "raw",
    ".mypy_cache",
    ".ruff_cache",
}

# Binary or generated suffixes we do not scan.
SKIP_SUFFIXES = {
    ".png",
    ".jpg",
    ".jpeg",
    ".pdf",
    ".gz",
    ".so",
    ".o",
    ".a",
    ".ico",
}

# The design specification is an external input document kept for provenance and
# is not repository prose we author, so it is exempt from the dash rule.
SKIP_FILES = {"llvm_pass_framework_spec.md"}

# LaTeX environments whose bodies are verbatim code, where "--" is a literal
# operator and not prose.
VERBATIM_ENVS = ("verbatim", "lstlisting", "minted", "Verbatim")


def tracked_files(root: Path) -> list[Path]:
    """Prefer git's view of the tree; fall back to a filesystem walk."""
    try:
        out = subprocess.run(
            ["git", "-C", str(root), "ls-files"],
            capture_output=True,
            text=True,
            check=True,
        )
        files = [root / line for line in out.stdout.splitlines() if line]
        if files:
            return files
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    return [p for p in root.rglob("*") if p.is_file()]


def should_skip(path: Path, root: Path) -> bool:
    rel = path.relative_to(root)
    if any(part in SKIP_DIRS for part in rel.parts):
        return True
    if path.suffix.lower() in SKIP_SUFFIXES:
        return True
    if path.name in SKIP_FILES:
        return True
    return False


def scan_file(path: Path) -> list[tuple[int, str]]:
    """Return a list of (line_number, message) violations for one file."""
    try:
        text = path.read_text(encoding="utf-8")
    except (UnicodeDecodeError, OSError):
        # Not UTF-8 text; treat as binary and ignore.
        return []

    violations: list[tuple[int, str]] = []
    is_tex = path.suffix == ".tex"
    in_verbatim = False

    for lineno, line in enumerate(text.splitlines(), start=1):
        if EM_DASH in line:
            violations.append((lineno, "em dash U+2014"))
        if EN_DASH in line:
            violations.append((lineno, "en dash U+2013"))

        if not is_tex:
            continue

        stripped = line.strip()
        if stripped.startswith("\\begin{") and any(
            f"\\begin{{{env}}}" in stripped for env in VERBATIM_ENVS
        ):
            in_verbatim = True
            continue
        if stripped.startswith("\\end{") and any(
            f"\\end{{{env}}}" in stripped for env in VERBATIM_ENVS
        ):
            in_verbatim = False
            continue
        if in_verbatim:
            continue

        # Strip a trailing LaTeX comment before checking prose, but a comment is
        # still prose we author, so we check the whole line for dash runs.
        if "--" in line:
            violations.append((lineno, 'literal "--" or "---" in .tex prose'))

    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        default=str(Path(__file__).resolve().parent.parent),
        help="repository root to scan",
    )
    args = parser.parse_args()
    root = Path(args.root).resolve()

    total = 0
    for path in sorted(tracked_files(root)):
        if not path.is_file() or should_skip(path, root):
            continue
        for lineno, message in scan_file(path):
            rel = path.relative_to(root)
            print(f"{rel}:{lineno}: {message}")
            total += 1

    if total:
        print(f"\ncheck_no_dashes: {total} violation(s) found", file=sys.stderr)
        return 1
    print("check_no_dashes: clean")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
