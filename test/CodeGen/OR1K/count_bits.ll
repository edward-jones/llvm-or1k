; RUN: llc -march=or1k -mattr=fbit < %s | FileCheck %s

declare i8 @llvm.cttz.i8(i8, i1)
declare i16 @llvm.cttz.i16(i16, i1)
declare i32 @llvm.cttz.i32(i32, i1)

declare i8 @llvm.ctlz.i8(i8, i1)
declare i16 @llvm.ctlz.i16(i16, i1)
declare i32 @llvm.ctlz.i32(i32, i1)

define i32 @cttz32(i32 %a) {
  %1 = call i32 @llvm.cttz.i32(i32 %a, i1 true)
  ret i32 %1
}
; CHECK: cttz32:
; CHECK: l.ff1

define i32 @ctlz32(i32 %a) {
  %1 = call i32 @llvm.ctlz.i32(i32 %a, i1 true)
  ret i32 %1
}
; CHECK: ctlz32:
; CHECK: l.fl1

define i16 @cttz16(i16 %a) {
  %1 = call i16 @llvm.cttz.i16(i16 %a, i1 true)
  ret i16 %1
}
; CHECK: cttz16:
; CHECK: l.ff1

define i16 @ctlz16(i16 %a) {
  %1 = call i16 @llvm.ctlz.i16(i16 %a, i1 true)
  ret i16 %1
}
; CHECK: ctlz16:
; CHECK: l.fl1

define i8 @cttz8(i8 %a) {
  %1 = call i8 @llvm.cttz.i8(i8 %a, i1 true)
  ret i8 %1
}
; CHECK: cttz8:
; CHECK: l.ff1

define i8 @ctlz8(i8 %a) {
  %1 = call i8 @llvm.ctlz.i8(i8 %a, i1 true)
  ret i8 %1
}
; CHECK: ctlz8:
; CHECK: l.fl1
