; RUN: llc -march=or1k < %s | FileCheck %s

define i32 @zextloadi8(i8* %a.addr) {
  %1 = load i8* %a.addr
  %2 = zext i8 %1 to i32
  ret i32 %2
}
; CHECK: zextloadi8:
; CHECK: l.lbz

define i32 @zextloadi16(i16* %a.addr) {
  %1 = load i16* %a.addr
  %2 = zext i16 %1 to i32
  ret i32 %2
}
; CHECK: zextloadi16:
; CHECK: l.lhz

define i32 @sextloadi8(i8* %a.addr) {
  %1 = load i8* %a.addr
  %2 = sext i8 %1 to i32
  ret i32 %2
}
; CHECK: sextloadi8:
; CHECK: l.lbs

define i32 @sextloadi16(i16* %a.addr) {
  %1 = load i16* %a.addr
  %2 = sext i16 %1 to i32
  ret i32 %2
}
; CHECK: sextloadi16:
; CHECK: l.lhs
