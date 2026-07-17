; The opcode analysis counts instructions by mnemonic. Two adds, one mul, one
; ret, four instructions across three opcodes.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-print-opcode-stats" -disable-output "%s" | FileCheck "%s"

define i32 @add_chain(i32 %x) {
  %a = add i32 %x, 1
  %b = add i32 %a, 2
  %c = mul i32 %b, %b
  ret i32 %c
}

; CHECK: === opcode stats for function 'add_chain' ===
; CHECK: opcode stats: 4 instructions across 3 opcodes
; CHECK-DAG: 2  add
; CHECK-DAG: 1  mul
; CHECK-DAG: 1  ret
