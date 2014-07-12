//===-- OR1KRegisterInfo.cpp - OR1K Register Information --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the OR1K implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "OR1K.h"
#include "OR1KFrameLowering.h"
#include "OR1KMachineFunctionInfo.h"
#include "OR1KRegisterInfo.h"
#include "OR1KSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetInstrInfo.h"

#define DEBUG_TYPE "or1k-register-info"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "OR1KGenRegisterInfo.inc"

OR1KRegisterInfo::OR1KRegisterInfo(const TargetInstrInfo &tii)
  : OR1KGenRegisterInfo(OR1K::R9), TII(tii) {}

const uint16_t *
OR1KRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_SaveList;
}

bool OR1KRegisterInfo::hasReservedGlobalBaseRegister(
    const MachineFunction &MF) const {
  if (MF.getTarget().getRelocationModel() != Reloc::PIC_)
    return false;

  switch (MF.getTarget().getCodeModel()) {
  default:
    return true;
  case CodeModel::Medium:
  case CodeModel::Large:
    return false;
  }
}

unsigned OR1KRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();

  return TFI->hasFP(MF) ? OR1K::R2 : OR1K::R1;
}

unsigned OR1KRegisterInfo::getGlobalBaseRegister() const {
  return OR1K::R16;
}

unsigned OR1KRegisterInfo::getBaseRegister() const {
  return OR1K::R14;
}

BitVector OR1KRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  const TargetMachine &TM = MF.getTarget();
  auto TFI = static_cast<const OR1KFrameLowering *>(TM.getFrameLowering());

  BitVector Reserved(getNumRegs());

  Reserved.set(OR1K::R0);
  Reserved.set(OR1K::R1);
  Reserved.set(OR1K::R9);
  Reserved.set(OR1K::R10);

  Reserved.set(OR1K::MACLO);
  Reserved.set(OR1K::MACHI);

  if (TFI->hasFP(MF))
    Reserved.set(getFrameRegister(MF));

  if (TFI->hasBP(MF))
    Reserved.set(getBaseRegister());

  if (hasReservedGlobalBaseRegister(MF))
    Reserved.set(getGlobalBaseRegister());

  return Reserved;
}

bool
OR1KRegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

bool OR1KRegisterInfo::requiresFrameIndexScavenging(
    const MachineFunction &MF) const {
  return true;
}

void OR1KRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj, unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected");

  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();
  DebugLoc dl = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();

  int Offset = MFI->getObjectOffset(FrameIndex) +
               MI.getOperand(FIOperandNum + 1).getImm();

  bool UsePreviousSP = FuncInfo->isFrameIndexUsingPreviousSP(FrameIndex);
  bool HasRealignedStack = needsStackRealignment(MF);
  bool IsLocalObject = FrameIndex >= 0;

  bool UsesBP = MFI->hasVarSizedObjects() && IsLocalObject && HasRealignedStack;

  bool NeedsPositiveOffset =
      !TFI->hasFP(MF) || (IsLocalObject && HasRealignedStack);

  if (NeedsPositiveOffset && !UsePreviousSP)
    Offset += MFI->getStackSize();

  unsigned FrameReg = 0;
  if (UsesBP)
    FrameReg = getBaseRegister();
  else if (UsePreviousSP || NeedsPositiveOffset)
    FrameReg = OR1K::R1;
  else
    FrameReg = getFrameRegister(MF);

  if (!isInt<16>(Offset)) {
    unsigned VReg = MRI.createVirtualRegister(&OR1K::GPRRegClass);

    // l.movhi rT, hi(offset)
    // l.ori rT, rT, lo(offset)
    // l.add rT, rT, rF
    BuildMI(MBB, II, dl, TII.get(OR1K::MOVHI), VReg)
        .addImm((Offset >> 16) & 0xFFFFU);
    BuildMI(MBB, II, dl, TII.get(OR1K::ORI), VReg)
     .addReg(VReg).addImm(Offset & 0xFFFFU);
    BuildMI(MBB, II, dl, TII.get(OR1K::ADD), VReg)
     .addReg(VReg).addReg(FrameReg);

    // Use value in rT as effective address.
    MI.getOperand(FIOperandNum).ChangeToRegister(VReg, false, false, true);
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);

    return;
  }

  MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
}

bool OR1KRegisterInfo::needsStackRealignment(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const Function *F = MF.getFunction();
  unsigned StackAlign = MF.getTarget().getFrameLowering()->getStackAlignment();
  return ((MFI->getMaxAlignment() > StackAlign) ||
          F->getAttributes().hasAttribute(AttributeSet::FunctionIndex,
                                          Attribute::StackAlignment));
}

bool OR1KRegisterInfo::hasReservedSpillSlot(const MachineFunction &MF,
                                            unsigned Reg,
                                            int &FrameIndex) const {
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();
  switch (Reg) {
  default:
    break;
  case OR1K::R9:
    FrameIndex = FuncInfo->getReturnAddressFI();
    return FuncInfo->hasReturnAddressStackSlot();
  case OR1K::R2:
    FrameIndex = FuncInfo->getFramePointerFI();
    return FuncInfo->hasFramePointerStackSlot();
  }
  return false;
}
