; Cases the simplifier must leave exactly as they are. Getting these wrong would
; be a miscompile, so they are as important as the positive rewrites.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-simplify" -S "%s" | FileCheck "%s"

; Floating point add of zero is not an identity without fast math flags, because
; x + 0.0 differs from x when x is negative zero.
define float @fadd_zero(float %x) {
  %r = fadd float %x, 0.0
  ret float %r
}
; CHECK-LABEL: @fadd_zero(
; CHECK: fadd float %x, 0

; Multiply by a non power of two is not strength reducible to a single shift.
define i32 @mul_three(i32 %x) {
  %r = mul i32 %x, 3
  ret i32 %r
}
; CHECK-LABEL: @mul_three(
; CHECK: mul i32 %x, 3

; We do not do constant folding, so two constant operands stay put.
define i32 @const_fold_left_alone() {
  %r = add i32 2, 3
  ret i32 %r
}
; CHECK-LABEL: @const_fold_left_alone(
; CHECK: add i32 2, 3

; Division by one is outside the identity set this pass claims, so it is
; untouched (the pass never asserts more than it implements).
define i32 @sdiv_one(i32 %x) {
  %r = sdiv i32 %x, 1
  ret i32 %r
}
; CHECK-LABEL: @sdiv_one(
; CHECK: sdiv i32 %x, 1
