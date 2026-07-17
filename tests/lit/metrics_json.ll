; The combined metrics emitter produces one JSON document with per function
; blocks and module wide totals. Validate the structural fields the harness
; relies on.
;
; RUN: opt -load-pass-plugin="%opflib" -passes="my-metrics-json" -disable-output "%s" | FileCheck "%s"

define i32 @f(i32 %x) {
  %a = add i32 %x, %x
  ret i32 %a
}

; CHECK: "functions": [
; CHECK: "name": "f"
; CHECK: "totals": {
; CHECK: "num_functions": 1
; CHECK: "total_instructions": 2
