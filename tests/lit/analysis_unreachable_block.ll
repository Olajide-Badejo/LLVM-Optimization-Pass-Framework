; A block with no predecessors other than entry counts as unreachable. Here the
; "dead" block is never branched to, so the analysis must report exactly one
; unreachable block.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-print-block-stats" -disable-output "%s" | FileCheck "%s"

define void @has_dead_block(i1 %c) {
entry:
  br i1 %c, label %live, label %exit
live:
  br label %exit
dead:
  br label %exit
exit:
  ret void
}

; CHECK: === block stats for function 'has_dead_block' ===
; CHECK: unreachable blocks: 1
