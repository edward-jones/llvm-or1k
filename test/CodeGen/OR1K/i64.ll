; RUN: llc -march=or1k -mattr=mul,mul64 < %s | FileCheck %s

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

define i64 @zmul64(i32 %a, i32 %b) #0 {
entry:
  %aext = zext i32 %a to i64
  %bext = zext i32 %b to i64
  %mul = mul i64 %aext, %bext
  ret i64 %mul
}
; CHECK: mul64:
; CHECK: l.muldu
; CHECK-NEXT: l.mfspr
; CHECK-NEXT: l.mfspr

define i64 @smul64(i32 %a, i32 %b) #0 {
entry:
  %aext = sext i32 %a to i64
  %bext = sext i32 %b to i64
  %mul = mul nsw i64 %aext, %bext
  ret i64 %mul
}
; CHECK: mul64:
; CHECK: l.muld
; CHECK-NEXT: l.mfspr
; CHECK-NEXT: l.mfspr
