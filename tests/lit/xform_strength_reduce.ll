; Multiply by a constant power of two becomes a shift, and the no wrap flags are
; carried onto the shift.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-simplify" -S "%s" | FileCheck "%s"

define i32 @mul_eight(i32 %x) {
  %r = mul i32 %x, 8
  ret i32 %r
}
; CHECK-LABEL: @mul_eight(
; CHECK-NOT: mul
; CHECK: shl i32 %x, 3

define i32 @mul_sixteen_nsw(i32 %x) {
  %r = mul nsw i32 %x, 16
  ret i32 %r
}
; CHECK-LABEL: @mul_sixteen_nsw(
; CHECK: shl nsw i32 %x, 4

define i32 @mul_two_nuw(i32 %x) {
  %r = mul nuw i32 %x, 2
  ret i32 %r
}
; CHECK-LABEL: @mul_two_nuw(
; CHECK: shl nuw i32 %x, 1
