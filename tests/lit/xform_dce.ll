; DCE removes trivially dead instructions and leaves anything with a side effect
; alone. The negative cases are the point: a store, a volatile load, and a call
; must all survive.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-dce" -S "%s" | FileCheck "%s"

define i32 @removes_chain(i32 %x) {
  %dead1 = add i32 %x, 1
  %dead2 = mul i32 %dead1, %dead1
  %live = sub i32 %x, 3
  ret i32 %live
}
; CHECK-LABEL: @removes_chain(
; CHECK-NOT: add
; CHECK-NOT: mul
; CHECK: ret i32 %live

define void @keeps_store(ptr %p, i32 %x) {
  %dead = add i32 %x, 7
  store i32 %x, ptr %p
  ret void
}
; CHECK-LABEL: @keeps_store(
; CHECK-NOT: add
; CHECK: store i32 %x

define i32 @keeps_volatile(ptr %p) {
  %v = load volatile i32, ptr %p
  ret i32 0
}
; CHECK-LABEL: @keeps_volatile(
; CHECK: load volatile

declare void @sink(i32)

define void @keeps_call(i32 %x) {
  call void @sink(i32 %x)
  ret void
}
; CHECK-LABEL: @keeps_call(
; CHECK: call void @sink
