; RUN: llc -march=or1k -mattr=ext < %s | FileCheck %s

declare void @llvm.or1k.msync()
declare void @llvm.or1k.psync()
declare void @llvm.or1k.csync()

define void @synchronization() {
  call void @llvm.or1k.msync()
  call void @llvm.or1k.psync()
  call void @llvm.or1k.csync()
  ret void
}
; CHECK: l.msync
; CHECK-NEXT: l.psync
; CHECK-NEXT: l.csync

declare i32 @llvm.or1k.mfspr(i32)
declare void @llvm.or1k.mtspr(i32, i32)

define void @special_registers_immediate() {
  %val = call i32 @llvm.or1k.mfspr(i32 10241)
  call void @llvm.or1k.mtspr(i32 %val, i32 10241)
  ret void
}
; CHECK: l.mfspr [[FREG:r[0-9]+]], r0, 10241
; CHECK-NEXT: l.mtspr r0, [[TREG:r[0-9]+]], 10241

define void @special_registers_variable(i32 %reg_no) {
  %val = call i32 @llvm.or1k.mfspr(i32 %reg_no)
  call void @llvm.or1k.mtspr(i32 %val, i32 %reg_no)
  ret void
}
; CHECK: l.mfspr [[FREG:r[0-9]+]], [[FSCRATCHREG:r[0-9]+]], 0
; CHECK-NEXT: l.mtspr [[DSCRATCHREG:r[0-9]+]], [[TREG:r[0-9]+]], 0
