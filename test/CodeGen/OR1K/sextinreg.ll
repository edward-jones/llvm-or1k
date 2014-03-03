; RUN: llc -march=or1k < %s | FileCheck %s

define i32 @u8tou32(i8 %val) {
  %1 = zext i8 %val to i32
  ret i32 %1
}
; CHECK: u8tou32:
; CHECK: l.extbz

define i32 @u16tou32(i16 %val) {
  %1 = zext i16 %val to i32
  ret i32 %1
}
; CHECK: u16tou32:
; CHECK: l.exthz

define i32 @s8tos32(i8 %val) {
  %1 = sext i8 %val to i32
  ret i32 %1
}
; CHECK: s8tos32:
; CHECK: l.extbs

define i32 @s16tos32(i16 %val) {
  %1 = sext i16 %val to i32
  ret i32 %1
}
; CHECK: s16tos32:
; CHECK: l.exths
