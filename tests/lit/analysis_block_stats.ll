; The block analysis reports counts, sizes, terminator kinds, and a size
; histogram. This diamond has four single terminator blocks: three end in br,
; one in ret, and none are unreachable.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-print-block-stats" -disable-output "%s" | FileCheck "%s"

define void @diamond(i1 %c) {
entry:
  br i1 %c, label %a, label %b
a:
  br label %exit
b:
  br label %exit
exit:
  ret void
}

; CHECK: === block stats for function 'diamond' ===
; CHECK: block stats: 4 blocks, 4 instructions
; CHECK: unreachable blocks: 0
; CHECK-DAG: 3  br
; CHECK-DAG: 1  ret
