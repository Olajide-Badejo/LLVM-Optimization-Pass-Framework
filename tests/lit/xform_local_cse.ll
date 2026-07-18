; Local CSE folds a repeated pure computation within a block into the first one.
; It is block scoped, so an identical computation in another block is left
; alone, and it does not touch loads.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-local-cse" -S "%s" | FileCheck "%s"

define i32 @same_block(i32 %x, i32 %y) {
  %a = add i32 %x, %y
  %b = add i32 %x, %y
  %c = mul i32 %a, %b
  ret i32 %c
}
; CHECK-LABEL: @same_block(
; CHECK: %a = add i32 %x, %y
; CHECK-NOT: %b = add
; CHECK: mul i32 %a, %a

define i32 @cross_block(i32 %x, i32 %y, i1 %c) {
entry:
  %a = add i32 %x, %y
  br i1 %c, label %next, label %next
next:
  %b = add i32 %x, %y
  ret i32 %b
}
; CHECK-LABEL: @cross_block(
; CHECK: %a = add
; CHECK: %b = add

define i32 @keeps_loads(ptr %p) {
  %a = load i32, ptr %p
  %b = load i32, ptr %p
  %c = add i32 %a, %b
  ret i32 %c
}
; CHECK-LABEL: @keeps_loads(
; CHECK: %a = load
; CHECK: %b = load
