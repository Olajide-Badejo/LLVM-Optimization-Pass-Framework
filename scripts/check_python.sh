#!/usr/bin/env bash
# Lint the Python sources with ruff. Prefers the harness virtualenv, falls back
# to a ruff on PATH, and skips gracefully if neither exists so the style target
# still runs on a machine without the Python tooling installed.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_RUFF="${ROOT}/benchmarks/harness/.venv/bin/ruff"

if [[ -x "${VENV_RUFF}" ]]; then
  RUFF="${VENV_RUFF}"
elif command -v ruff >/dev/null 2>&1; then
  RUFF="ruff"
else
  echo "ruff: not installed; skipping Python lint"
  exit 0
fi

"${RUFF}" check "${ROOT}/scripts" "${ROOT}/benchmarks/harness"
echo "ruff: clean"
