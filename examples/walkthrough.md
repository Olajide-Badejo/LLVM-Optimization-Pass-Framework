# Annotated walkthroughs

These are real before and after snapshots, produced by the plugin. Build it
first with `make build`; the plugin is at `build/lib/libOPFPasses.so`. On the
pinned toolchain the tool is `opt-21`.

## 1. Algebraic identities and strength reduction

```bash
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so -passes="my-simplify" -S in.ll
```

Input:

```llvm
define i32 @simp(i32 %x, i32 %y) {
  %a = add i32 %x, 0     ; identity: add of zero
  %b = mul i32 %a, 8     ; strength reduce: multiply by a power of two
  %c = sub i32 %y, %y    ; identity: subtract of self is zero
  %d = or  i32 %b, %c    ; or of zero is the other operand
  ret i32 %d
}
```

Output:

```llvm
define i32 @simp(i32 %x, i32 %y) {
  %1 = shl i32 %x, 3
  ret i32 %1
}
```

The add of zero collapses to `%x`, the multiply by eight becomes a shift by
three, the subtract of self becomes zero, and the or of zero drops away. The
chain folds down to a single shift that computes the same value.

## 2. Canonicalization exposes common subexpressions

```bash
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so -passes="my-canon,my-local-cse" -S in.ll
```

Input:

```llvm
define i32 @cse(i32 %x, i32 %y) {
  %a = add i32 %x, %y
  %b = add i32 %y, %x    ; same computation, operands swapped
  %c = mul i32 %a, %b
  ret i32 %c
}
```

Output:

```llvm
define i32 @cse(i32 %x, i32 %y) {
  %a = add i32 %x, %y
  %c = mul i32 %a, %a
  ret i32 %c
}
```

Canonicalization rewrites `add %y, %x` into `add %x, %y`, so local CSE can see
that the two adds are the same computation and fold the second into the first.
This is why the pipeline runs canonicalize before CSE.

## 3. The full pipeline

```bash
opt-21 -load-pass-plugin=build/lib/libOPFPasses.so -passes="my-default-verbose" \
       -S examples/composite.ll
```

The transformed IR goes to stdout and a per iteration log to stderr:

```text
=== my-default pipeline on 'composite' ===
iteration 1: my-canon=1 my-simplify=5 my-local-cse=1 my-dce=0 (total 7)
iteration 2: my-canon=0 my-simplify=0 my-local-cse=0 my-dce=0 (total 0)
reached fixed point after 2 iterations, 7 rewrites total
```

The second iteration changes nothing, which is how the pipeline knows it has
reached a fixed point. The result is a single shift feeding one add, computing
eight times the input, with every identity, redundancy, and dead instruction
gone.
