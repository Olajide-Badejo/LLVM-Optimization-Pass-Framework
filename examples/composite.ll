; A small module that exercises the whole pipeline. Run it with:
;
;   opt-21 -load-pass-plugin=build/lib/libOPFPasses.so \
;          -passes="my-default-verbose" -S examples/composite.ll
;
; See examples/walkthrough.md for the annotated before and after.

define i32 @composite(i32 %x, i32 %y) {
  %a = add i32 %x, 0
  %b = add i32 0, %x
  %c = mul i32 %a, 4
  %d = mul i32 %b, 4
  %e = add i32 %c, %d
  %dead = xor i32 %y, %y
  ret i32 %e
}
