; RUN: llc -march=or1k < %s | FileCheck %s

define i64 @add64rr(i64 %a, i64 %b) {
entry:
  %result = add i64 %b, %a
  ret i64 %result
}
; CHECK: add64rr:
; CHECK: l.add
; CHECK: l.addc


define i64 @addi64ri32(i64 %a) {
entry:
  %result = add i64 %a, 42
  ret i64 %result
}
; CHECK: addi64ri32:
; CHECK: l.addi
; CHECK: l.addic

define i64 @addi64ri64(i64 %a) {
entry:
  %result = add i64 %a, 42000000000
  ret i64 %result
}
; CHECK: addi64ri64:
; CHECK: l.add
; CHECK: l.addic
