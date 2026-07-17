; Phase 0 gate: the plugin loads into a stock opt and the my-noop pass runs
; over a real module without disturbing it. This is the smallest possible
; end to end proof that registration works.
;
; Substitutions are quoted so the suite passes even when the checkout path
; contains spaces, which is common on Windows hosts running under WSL.
; RUN: opt -load-pass-plugin="%opflib" -passes="my-noop" -S "%s" | FileCheck "%s"

define i32 @identity(i32 %x) {
entry:
  ret i32 %x
}

; CHECK-LABEL: define i32 @identity(i32 %x)
; CHECK: entry:
; CHECK-NEXT: ret i32 %x
