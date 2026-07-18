# Passes

## Analyses

### Opcode stats (`my-print-opcode-stats`, `OpcodeStatsAnalysis`)

Counts instructions by LLVM opcode mnemonic and the total. A linear walk of the
function. Used as the headline structural metric.

### Block stats (`my-print-block-stats`, `BlockStatsAnalysis`)

Reports block count, a block size histogram, min, mean, and max block size,
terminator kinds, and the number of unreachable blocks (blocks with no
predecessor other than entry).

### Dead code report (`my-print-dead-code`, `DeadCodeAnalysis`)

Counts trivially dead instructions and the longest use-def chain depth. An
instruction is trivially dead when it feeds nothing, ends no block, and has no
side effects. Chain depth is the longest run of data dependencies; phi nodes are
treated as chain roots so a loop carried dependency does not create an unbounded
chain. This is where objective one's use-def chain metric lives.

### Simplify opportunities (`my-print-simplify-opps`, `SimplifyOpportunitiesAnalysis`)

Counts, without changing anything, the patterns the transforms will act on:
constant foldable binary ops, algebraic identities, strength reducible
multiplies, and block local duplicate pure computations. It shares its
predicates with the transforms, so it doubles as a check that the transforms
found everything they should have.

## Transforms

Every transform is conservative: only provably safe rewrites, scalar integers
only, and it never touches floating point (which needs fast math flags),
vectors, volatile, or atomic operations. Each has its own lit test with the
exact before and after, plus negative tests proving the unsafe cases are left
alone.

### Canonicalize (`my-canon`)

Reorders the operands of commutative instructions into a stable form: constants
to the right, other operands by definition order. Swapping commutative operands
never changes a value, so this is always safe. It exposes duplicates for CSE and
puts constants where the simplifier looks for them.

Before and after:

```
%r = add i32 0, %x      becomes      %r = add i32 %x, 0
```

### Algebraic simplify (`my-simplify`)

Folds integer identities and strength reduces a multiply by a constant power of
two into a shift, carrying the no wrap flags forward.

| Pattern | Result |
|---|---|
| `add x, 0` | `x` |
| `sub x, 0`, `sub x, x` | `x`, `0` |
| `mul x, 1`, `mul x, 0` | `x`, `0` |
| `mul x, 2^k` (k > 0) | `shl x, k` |
| `and x, -1`, `and x, 0`, `and x, x` | `x`, `0`, `x` |
| `or x, 0`, `or x, -1`, `or x, x` | `x`, `-1`, `x` |
| `xor x, 0`, `xor x, x` | `x`, `0` |
| `shl/lshr/ashr x, 0` | `x` |

Why safe: each rewrite is value preserving on scalar integers regardless of the
wrap flags, and strength reduction transfers `nsw` and `nuw` onto the shift,
which overflows exactly when the multiply did. Floating point is untouched
because the predicates only match integer constants.

```
%r = mul nsw i32 %x, 16     becomes      %r = shl nsw i32 %x, 4
```

### Local CSE (`my-local-cse`)

Within one basic block, replaces a pure computation that repeats an earlier
identical one with a reference to the first. Two instructions match when their
structural key matches, and canonicalization has already normalized commutative
operand order, so `add a, b` and `add b, a` are recognized as the same. Loads
are never deduplicated because two identical loads can observe different memory.

```
%a = add i32 %x, %y
%b = add i32 %x, %y     ->  uses of %b become %a, %b removed
```

### DCE (`my-dce`)

Removes trivially dead instructions, using a worklist so that removing one use
can make its operands dead in turn. Anything with a side effect (a store, a
volatile or atomic access, a call that is not known side effect free) is kept.

## The default pipeline (`my-default`)

Runs canonicalize, simplify, local CSE, DCE in that order, repeating to a fixed
point with an eight iteration cap. `my-default-verbose` prints a per iteration
log naming each pass and its rewrite count and whether a fixed point was
reached. See `docs/architecture.md` for why the order is what it is.
