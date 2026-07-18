// dead_code: a loop that computes a chain of scalar values that never reach the
// result. At -O0 clang emits all of them; after mem2reg they become dead SSA
// instructions feeding only each other, which the framework's DCE removes back
// to front. The one live value is what the checksum depends on, so the
// transformed binary must still print the same number.

#include <stdint.h>
#include <stdio.h>

static uint64_t kernel(uint32_t n, uint32_t seed) {
  uint64_t acc = 0;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t x = i + seed;

    // A chain of computations that nothing observable ever uses.
    uint32_t dead1 = x * 2654435761u;
    uint32_t dead2 = dead1 ^ 0xA5A5A5A5u;
    uint32_t dead3 = dead2 + x;
    uint32_t dead4 = dead3 * dead1;
    (void)dead4;

    uint32_t live = x + 1u;
    acc += live;
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
