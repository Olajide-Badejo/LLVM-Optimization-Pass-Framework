#!/usr/bin/env bash
# Check (or with --fix, apply) clang-format over the C++ sources.
set -euo pipefail

LLVM_MAJOR="${OPF_LLVM_MAJOR:-21}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CF=""
for cand in "clang-format-${LLVM_MAJOR}" clang-format; do
  if command -v "${cand}" >/dev/null 2>&1; then CF="${cand}"; break; fi
done
if [[ -z "${CF}" ]]; then
  echo "error: clang-format not found" >&2
  exit 1
fi

mapfile -t files < <(
  find "${ROOT}/lib" "${ROOT}/include" "${ROOT}/tests/unit" \
    -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) 2>/dev/null | sort
)

if [[ ${#files[@]} -eq 0 ]]; then
  echo "clang-format: no sources to check"
  exit 0
fi

if [[ "${1:-}" == "--fix" ]]; then
  "${CF}" -i "${files[@]}"
  echo "clang-format: applied to ${#files[@]} file(s)"
  exit 0
fi

status=0
for f in "${files[@]}"; do
  if ! diff -q <("${CF}" "${f}") "${f}" >/dev/null; then
    echo "needs formatting: ${f#"${ROOT}/"}"
    status=1
  fi
done
if [[ ${status} -eq 0 ]]; then
  echo "clang-format: clean (${#files[@]} file(s))"
fi
exit ${status}
