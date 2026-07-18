# Usage

Build the plugin first (`make build`); it lands at
`build/lib/libOPFPasses.so`. Every invocation loads it into a stock `opt` with
`-load-pass-plugin`. On the pinned toolchain the tool is `opt-21`.

## Registered pass names

Function passes (run with `-passes=...` over each function):

| Name | What it does |
|---|---|
| `my-noop` | does nothing; the smallest proof the plugin loaded |
| `my-print-opcode-stats` | prints instruction counts by opcode |
| `my-print-block-stats` | prints block counts, size histogram, terminators |
| `my-print-dead-code` | prints dead instruction count and use-def chain depth |
| `my-print-simplify-opps` | prints simplification opportunities by category |
| `my-canon` | orders commutative operands into a canonical form |
| `my-simplify` | folds algebraic identities and strength reduces multiplies |
| `my-local-cse` | removes block local duplicate pure computations |
| `my-dce` | removes trivially dead instructions |
| `my-default` | the four transforms iterated to a fixed point |
| `my-default-verbose` | `my-default` plus a per iteration log to stderr |

Module pass:

| Name | What it does |
|---|---|
| `my-metrics-json` | emits one JSON document of all metrics for the module |

## Examples

Run a single transform and print the result:

```
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
       -passes="my-simplify" -S input.ll
```

Compose the transforms by hand, in the same order the default pipeline uses:

```
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
       -passes="my-canon,my-simplify,my-local-cse,my-dce" -S input.ll
```

Run the fixed point pipeline and watch what each iteration did:

```
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
       -passes="my-default-verbose" -S input.ll
```

The iteration log goes to stderr, so the transformed IR on stdout stays clean
for piping or for FileCheck.

Print human readable analysis reports (analyses do not modify the IR, so pair
them with `-disable-output`):

```
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
       -passes="my-print-opcode-stats,my-print-block-stats" \
       -disable-output input.ll
```

Emit every metric as JSON for tooling:

```
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
       -passes="my-metrics-json" -disable-output input.ll
```

## A note on optnone

`clang -O0` marks functions `optnone`, which makes `opt` skip them entirely.
When starting from C, pass `-Xclang -disable-O0-optnone` to `clang` so the
functions stay optimizable. The benchmark harness does this for you.
