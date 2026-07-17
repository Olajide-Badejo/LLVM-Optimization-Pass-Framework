#!/usr/bin/env bash
# Configure and build the plugin against the pinned distribution LLVM.
set -euo pipefail

LLVM_MAJOR="${OPF_LLVM_MAJOR:-21}"
LLVM_CONFIG="llvm-config-${LLVM_MAJOR}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"

if ! command -v "${LLVM_CONFIG}" >/dev/null 2>&1; then
  echo "error: ${LLVM_CONFIG} not found on PATH." >&2
  echo "install: sudo apt-get install llvm-${LLVM_MAJOR}-dev clang-${LLVM_MAJOR} llvm-${LLVM_MAJOR}-tools" >&2
  exit 1
fi

CMAKE_DIR="$("${LLVM_CONFIG}" --cmakedir)"
echo "opf: building against LLVM $("${LLVM_CONFIG}" --version) at $("${LLVM_CONFIG}" --prefix)"

cmake -S "${ROOT}" -B "${BUILD}" -G Ninja \
  -DCMAKE_BUILD_TYPE="${OPF_BUILD_TYPE:-Release}" \
  -DLLVM_DIR="${CMAKE_DIR}" \
  -DOPF_LLVM_VERSION_MAJOR="${LLVM_MAJOR}"

cmake --build "${BUILD}" -j"$(nproc)"
echo "opf: plugin at ${BUILD}/lib/libOPFPasses.so"
