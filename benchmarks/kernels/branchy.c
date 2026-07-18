// branchy: control flow with algebraic identities inside each arm. The
// framework simplifies the arithmetic within the blocks (strength reduction and
// add identities) but deliberately does not fold branches or merge blocks, so
// this kernel is where a real -O1 pulls far ahead. The report uses it to make
// the honest point that these passes reimplement only a slice of the middle
// end. The checksum still has to match across all three builds.

#include <stdint.h>
#include <stdio.h>

static uint64_t kernel(uint32_t n, uint32_t seed) {
  uint64_t acc = 0;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t x = i ^ seed;
    uint32_t y;
    if ((x & 1u) == 0u) {
      y = x * 4u + 0u; // strength reduce and identity in the even arm
    } else if ((x & 2u) == 0u) {
      y = x * 2u - 0u; // strength reduce and identity in the middle arm
    } else {
      y = (x | 0u) + x; // identity in the odd arm
    }
    acc += y;
  }
  return acc;
}

int main(void) {
  uint64_t sum = 0;
  for (uint32_t r = 0; r < 1400; r++)
    sum += kernel(40000u, r);
  printf("%llu\n", (unsigned long long)sum);
  return 0;
}
