#!/usr/bin/env bash
# Set up the harness virtualenv (once) and run the benchmark suite. Extra
# arguments pass through to run_benchmarks.py, for example --force or
# --kernel arith_redundant.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HARNESS="${ROOT}/benchmarks/harness"
VENV="${HARNESS}/.venv"

if [[ ! -d "${VENV}" ]]; then
  echo "opf: creating harness virtualenv"
  python3 -m venv "${VENV}"
  "${VENV}/bin/pip" install --quiet --upgrade pip
  "${VENV}/bin/pip" install --quiet -r "${HARNESS}/requirements.txt"
fi

if [[ ! -f "${ROOT}/build/lib/libOPFPasses.so" ]]; then
  echo "opf: plugin not built; building first"
  bash "${ROOT}/scripts/build.sh"
fi

exec "${VENV}/bin/python" "${HARNESS}/run_benchmarks.py" "$@"
