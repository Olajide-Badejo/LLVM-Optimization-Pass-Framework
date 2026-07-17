; The dead code analysis counts instructions that feed nothing and have no side
; effects. Here the mul is unused and dead; the add feeds the return and lives.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-print-dead-code" -disable-output "%s" | FileCheck "%s"

define i32 @one_dead(i32 %x) {
  %live = add i32 %x, 1
  %dead = mul i32 %x, %x
  ret i32 %live
}

; CHECK: === dead code report for function 'one_dead' ===
; CHECK: dead code report: 1 of 3 instructions trivially dead
; CHECK: longest use-def chain: 2
