; my-default-verbose prints the per iteration change log to stderr (leaving the
; IR on stdout untouched), naming each pass and its rewrite count, and reports
; whether it reached a fixed point.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-default-verbose" -disable-output "%s" 2>&1 | FileCheck "%s"

define i32 @f(i32 %x) {
  %a = add i32 %x, 0
  %b = mul i32 %a, 8
  ret i32 %b
}

; CHECK: === my-default pipeline on 'f' ===
; CHECK: iteration 1: my-canon={{[0-9]+}} my-simplify={{[0-9]+}} my-local-cse={{[0-9]+}} my-dce={{[0-9]+}}
; CHECK: reached fixed point after {{[0-9]+}} iterations
