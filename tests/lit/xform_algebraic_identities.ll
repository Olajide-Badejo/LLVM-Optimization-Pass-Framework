; Every algebraic identity the simplifier recognizes, each proven by the exact
; before and after. The op under test must disappear and the result must equal
; the identity value.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-simplify" -S "%s" | FileCheck "%s"

define i32 @add_zero(i32 %x) {
  %r = add i32 %x, 0
  ret i32 %r
}
; CHECK-LABEL: @add_zero(
; CHECK-NOT: add
; CHECK: ret i32 %x

define i32 @sub_self(i32 %x) {
  %r = sub i32 %x, %x
  ret i32 %r
}
; CHECK-LABEL: @sub_self(
; CHECK-NOT: sub
; CHECK: ret i32 0

define i32 @mul_one(i32 %x) {
  %r = mul i32 %x, 1
  ret i32 %r
}
; CHECK-LABEL: @mul_one(
; CHECK-NOT: mul
; CHECK: ret i32 %x

define i32 @mul_zero(i32 %x) {
  %r = mul i32 %x, 0
  ret i32 %r
}
; CHECK-LABEL: @mul_zero(
; CHECK-NOT: mul
; CHECK: ret i32 0

define i32 @and_ones(i32 %x) {
  %r = and i32 %x, -1
  ret i32 %r
}
; CHECK-LABEL: @and_ones(
; CHECK-NOT: and
; CHECK: ret i32 %x

define i32 @and_zero(i32 %x) {
  %r = and i32 %x, 0
  ret i32 %r
}
; CHECK-LABEL: @and_zero(
; CHECK-NOT: and
; CHECK: ret i32 0

define i32 @or_zero(i32 %x) {
  %r = or i32 %x, 0
  ret i32 %r
}
; CHECK-LABEL: @or_zero(
; CHECK-NOT: or
; CHECK: ret i32 %x

define i32 @or_ones(i32 %x) {
  %r = or i32 %x, -1
  ret i32 %r
}
; CHECK-LABEL: @or_ones(
; CHECK-NOT: or
; CHECK: ret i32 -1

define i32 @xor_zero(i32 %x) {
  %r = xor i32 %x, 0
  ret i32 %r
}
; CHECK-LABEL: @xor_zero(
; CHECK-NOT: xor
; CHECK: ret i32 %x

define i32 @xor_self(i32 %x) {
  %r = xor i32 %x, %x
  ret i32 %r
}
; CHECK-LABEL: @xor_self(
; CHECK-NOT: xor
; CHECK: ret i32 0

define i32 @shl_zero(i32 %x) {
  %r = shl i32 %x, 0
  ret i32 %r
}
; CHECK-LABEL: @shl_zero(
; CHECK-NOT: shl
; CHECK: ret i32 %x
