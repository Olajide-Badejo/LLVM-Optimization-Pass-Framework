// arith_redundant: an arithmetic heavy loop with planted algebraic identities
// and redundant pure computations on scalar SSA values. After mem2reg promotes
// the locals to registers, the induction derived value x is a single SSA value,
// so the repeated x * 8 and x * x are genuine common subexpressions and the
// add 0 and or 0 are identities. This kernel is where the framework's simplify,
// local CSE, and DCE together should show a clear structural reduction.
//
// main prints a checksum so the harness can prove the transformed binary
// computes exactly what the baseline does.

#include <stdint.h>
#include <stdio.h>

static uint64_t kernel(uint32_t n, uint32_t seed) {
  uint64_t acc = 0;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t x = i ^ seed;
    uint32_t a = x * 8u + 0u;   // strength reduce and add identity
    uint32_t b = x * 8u;        // redundant with the multiply inside a
    uint32_t c = (x + 0u) | 0u; // two identities
    uint32_t d = x * x;
    uint32_t e = x * x;         // redundant with d
    acc += (uint64_t)a + b + c + d + e;
  }
  return acc;
}

int main(void) {
  uint64_t sum = 0;
  for (uint32_t r = 0; r < 1200; r++)
    sum += kernel(40000u, r);
  printf("%llu\n", (unsigned long long)sum);
  return 0;
}
