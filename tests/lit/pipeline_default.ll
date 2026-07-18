; The my-default pipeline composes all four transforms to a fixed point. This
; composite exercises the whole chain: canonicalization aligns the two adds,
; simplification folds the zero adds and strength reduces the multiplies, CSE
; merges the now identical shifts, and DCE clears the dead xor. What remains is
; a single shift feeding one add.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-default" -S "%s" | FileCheck "%s"

define i32 @composite(i32 %x, i32 %y) {
  %a = add i32 %x, 0
  %b = add i32 0, %x
  %c = mul i32 %a, 4
  %d = mul i32 %b, 4
  %e = add i32 %c, %d
  %dead = xor i32 %y, %y
  ret i32 %e
}

; CHECK-LABEL: @composite(
; CHECK-NOT: mul
; CHECK-NOT: xor
; CHECK-NOT: add i32 %x, 0
; CHECK: shl i32 %x, 2
; CHECK: ret i32
