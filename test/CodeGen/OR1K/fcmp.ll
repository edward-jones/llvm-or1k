; RUN: llc -march=or1k < %s | FileCheck %s
; RUN: llc -march=or1k -mattr=cmov < %s | FileCheck --check-prefix=CHECK-CMOV %s

define float @foeq(float %a, float %b) {
entry:
  %cmp = fcmp oeq float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: foeq:
; CHECK: lf.sfeq.s
; CHECK: l.bf


define float @fogt(float %a, float %b) {
entry:
  %cmp = fcmp ogt float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fogt:
; CHECK: lf.sfgt.s
; CHECK: l.bf


define float @foge(float %a, float %b) {
entry:
  %cmp = fcmp oge float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: foge:
; CHECK: lf.sfge.s
; CHECK: l.bf

define float @folt(float %a, float %b) {
entry:
  %cmp = fcmp olt float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: folt:
; CHECK: lf.sflt.s
; CHECK: l.bf

define float @fole(float %a, float %b) {
entry:
  %cmp = fcmp ole float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fole:
; CHECK: lf.sfle.s
; CHECK: l.bf

; CHECK-CMOV: fole:

define float @fone(float %a, float %b) {
entry:
  %cmp = fcmp one float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fone:
; CHECK: lf.sfgt.s
; CHECK: l.bf
; CHECK: lf.sflt.s
; CHECK: l.bf
; CHECK: l.or

; CHECK-CMOV: lf.sfgt.s r3, r4
; CHECK-CMOV: l.cmov [[REG_LT:r[0-9]+]], [[REG_ONE:r[0-9]+]], r0
; CHECK-CMOV: lf.sflt.s r3, r4
; CHECK-CMOV: l.cmov [[REG_GT:r[0-9]+]], [[REG_ONE]], r0
; CHECK-CMOV: l.or [[REG_R:r[0-9]+]], [[REG_GT]], [[REG_LT]]
; CHECK-CMOV: l.sfne [[REG_R]], r0


define float @ford(float %a, float %b) {
entry:
  %cmp = fcmp ord float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: ford:
; CHECK: lf.sfeq.s
; CHECK: l.bf
; CHECK: lf.sfeq.s
; CHECK: l.bf
; CHECK: l.and

; CHECK-CMOV: lf.sfeq.s r4, r4
; CHECK-CMOV: l.cmov [[REG_RHS:r[0-9]+]], [[REG_ONE:r[0-9]+]], r0
; CHECK-CMOV: lf.sfeq.s r3, r3
; CHECK-CMOV: l.cmov [[REG_LHS:r[0-9]+]], [[REG_ONE]], r0
; CHECK-CMOV: l.and [[REG_R:r[0-9]+]], [[REG_LHS]], [[REG_RHS]]
; CHECK-CMOV: l.sfne [[REG_R]], r0

define float @fueq(float %a, float %b) {
entry:
  %cmp = fcmp ueq float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fueq:
; CHECK: lf.sfgt.s
; CHECK: l.bf
; CHECK: lf.sflt.s
; CHECK: l.bf
; CHECK: l.or

; CHECK-CMOV: lf.sfgt.s r3, r4
; CHECK-CMOV: l.cmov [[REG_LT:r[0-9]+]], [[REG_ONE:r[0-9]+]], r0
; CHECK-CMOV: lf.sflt.s r3, r4
; CHECK-CMOV: l.cmov [[REG_GT:r[0-9]+]], [[REG_ONE]], r0
; CHECK-CMOV: l.or [[REG_R:r[0-9]+]], [[REG_GT]], [[REG_LT]]
; CHECK-CMOV: l.sfne [[REG_R]], r0

define float @fugt(float %a, float %b) {
entry:
  %cmp = fcmp ugt float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fugt:
; CHECK: lf.sfle.s
; CHECK: l.bf

define float @fuge(float %a, float %b) {
entry:
  %cmp = fcmp uge float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fuge:
; CHECK: lf.sflt.s
; CHECK: l.bf

define float @fult(float %a, float %b) {
entry:
  %cmp = fcmp ult float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fult:
; CHECK: lf.sfge.s
; CHECK: l.bf

define float @fule(float %a, float %b) {
entry:
  %cmp = fcmp ule float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fule:
; CHECK: lf.sfgt.s
; CHECK: l.bf

define float @fune(float %a, float %b) {
entry:
  %cmp = fcmp une float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: fune:
; CHECK: lf.sfne.s
; CHECK: l.bf

define float @funo(float %a, float %b) {
entry:
  %cmp = fcmp uno float %a, %b
  %a.b = select i1 %cmp, float %a, float %b
  ret float %a.b
}
; CHECK-LABEL: funo:
; CHECK: lf.sfeq.s
; CHECK: l.bf
; CHECK: lf.sfeq.s
; CHECK: l.bf
; CHECK: l.and
