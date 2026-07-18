// clean: a tight linear congruential loop with no identities, no redundant
// computations, and no dead code. The framework is expected to make no
// structural change here, and the report presents that as an honest negative
// result: a pass set that only fires when there is genuinely something to do.
// The multiplier is odd and not a power of two, the shift is nonzero, and every
// value feeds the result.

#include <stdint.h>
#include <stdio.h>

static uint64_t kernel(uint32_t n, uint32_t seed) {
  uint64_t acc = 0;
  uint32_t x = seed;
  for (uint32_t i = 0; i < n; i++) {
    x = x * 1664525u + 1013904223u;
    acc += x >> 3;
  }
  return acc;
}

int main(void) {
  uint64_t sum = 0;
  for (uint32_t r = 0; r < 1600; r++)
    sum += kernel(40000u, r);
  printf("%llu\n", (unsigned long long)sum);
  return 0;
}
