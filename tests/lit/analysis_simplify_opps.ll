; The simplify analysis counts opportunities by category without changing the
; IR. This function has one algebraic identity (add x, 0), one strength
; reducible multiply (mul x, 16), one constant foldable op (add 2, 3), and a
; duplicated pure add.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-print-simplify-opps" -disable-output "%s" | FileCheck "%s"

define i32 @opps(i32 %x, i32 %y) {
  %id  = add i32 %x, 0
  %sr  = mul i32 %x, 16
  %cf  = add i32 2, 3
  %da  = add i32 %x, %y
  %db  = add i32 %x, %y
  ret i32 %id
}

; CHECK: === simplify opportunities for function 'opps' ===
; CHECK: constant foldable:   1
; CHECK: algebraic identity:  1
; CHECK: strength reducible:  1
; CHECK: duplicate pure:      1
