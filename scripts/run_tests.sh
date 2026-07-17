#!/usr/bin/env bash
# Build, then run the unit (GoogleTest via ctest) and lit/FileCheck suites.
set -euo pipefail

LLVM_MAJOR="${OPF_LLVM_MAJOR:-21}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"

bash "${ROOT}/scripts/build.sh"

echo
echo "=== ctest (unit tests) ==="
# ctest also drives the lit suite through the registered "lit" test.
ctest --test-dir "${BUILD}" --output-on-failure
