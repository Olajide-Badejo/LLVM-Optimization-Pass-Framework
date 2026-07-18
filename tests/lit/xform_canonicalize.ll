; Canonicalization moves constants to the right hand side and orders other
; commutative operands by definition order. It never changes a value.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-canon" -S "%s" | FileCheck "%s"

define i32 @const_to_rhs(i32 %x) {
  %r = add i32 0, %x
  ret i32 %r
}
; CHECK-LABEL: @const_to_rhs(
; CHECK: add i32 %x, 0

define i32 @arg_order(i32 %x, i32 %y) {
  %r = add i32 %y, %x
  ret i32 %r
}
; CHECK-LABEL: @arg_order(
; CHECK: add i32 %x, %y

; A non commutative subtract must not be reordered; that would change the value.
define i32 @sub_untouched(i32 %x, i32 %y) {
  %r = sub i32 %y, %x
  ret i32 %r
}
; CHECK-LABEL: @sub_untouched(
; CHECK: sub i32 %y, %x

; Two constants are left as they are.
define i32 @two_constants() {
  %r = add i32 3, 5
  ret i32 %r
}
; CHECK-LABEL: @two_constants(
; CHECK: add i32 3, 5
