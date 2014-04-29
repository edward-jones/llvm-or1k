; RUN: llc -march=or1k < %s | FileCheck %s

define i32 @ret0() {
entry:
  ret i32 0
}

; CHECK: ret0:
; CHECK: l.ori r11, r0, 0
