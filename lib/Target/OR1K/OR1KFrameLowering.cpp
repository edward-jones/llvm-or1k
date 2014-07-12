//===-- OR1KFrameLowering.cpp - OR1K Frame Lowering -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the OR1K implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "OR1KFrameLowering.h"
#include "OR1KInstrInfo.h"
#include "OR1KMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"

#define DEBUG_TYPE "or1k-frame-lowering"

using namespace llvm;

bool OR1KFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = MF.getTarget().getRegisterInfo();

  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI->hasVarSizedObjects() || MFI->isFrameAddressTaken() ||
         TRI->needsStackRealignment(MF);
}

bool OR1KFrameLowering::hasBP(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = MF.getTarget().getRegisterInfo();

  return MFI->hasVarSizedObjects() && TRI->needsStackRealignment(MF);
}

bool OR1KFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  return !MFI->hasVarSizedObjects();
}

void OR1KFrameLowering::emitSPUpdate(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator &MBBI,
                                     DebugLoc DL, const TargetInstrInfo &TII,
                                     unsigned StackSize, unsigned DestReg,
                                     unsigned TmpReg, bool OnEpilogue) const {
  const unsigned SPReg = OR1K::R1;

  int64_t StackSizeAddend = OnEpilogue ? StackSize : -(int64_t)StackSize;

  if (StackSize == 0) {
    if (DestReg != SPReg)
      BuildMI(MBB, MBBI, DL, TII.get(OR1K::ORI), DestReg)
       .addReg(SPReg).addImm(0);

    return;
  }

  if (isInt<16>(StackSizeAddend)) {
    // l.addi rD, r1, -stack_size
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::ADDI), DestReg)
     .addReg(SPReg).addImm(StackSizeAddend);
    return;
  }

  // l.movhi rT, hi(stack_size)
  // l.ori rT, rT, lo(stack_size)
  BuildMI(MBB, MBBI, DL, TII.get(OR1K::MOVHI), TmpReg).addImm(StackSize >> 16);
  BuildMI(MBB, MBBI, DL, TII.get(OR1K::ORI), TmpReg)
    .addReg(TmpReg).addImm(StackSize & 0xFFFFU);

  if (OnEpilogue)
    // l.add rD, r1, rT
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::ADD), DestReg)
     .addReg(SPReg).addReg(TmpReg);
  else
    // l.sub rD, r1, rT
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::SUB), DestReg)
     .addReg(SPReg).addReg(TmpReg);
}

void OR1KFrameLowering::emitPrologue(MachineFunction &MF) const {
  MachineBasicBlock &MBB = MF.front();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetMachine &TM = MF.getTarget();
  const TargetInstrInfo &TII = *TM.getInstrInfo();
  auto TRI = static_cast<const OR1KRegisterInfo *>(TM.getRegisterInfo());
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  unsigned StackSize = MFI->getStackSize();

  const unsigned SPReg = OR1K::R1;
  const TargetRegisterClass *RC = &OR1K::GPRRegClass;

  if (FuncInfo->hasReturnAddressStackSlot()) {
    unsigned RAReg = TRI->getRARegister();
    int FI = FuncInfo->getReturnAddressFI();

    // l.sw ra_ss(r1), r9
    TII.storeRegToStackSlot(MBB, MBBI, RAReg, true, FI, RC, TRI);
  }

  if (FuncInfo->hasFramePointerStackSlot()) {
    unsigned FPReg = TRI->getFrameRegister(MF);
    int FI = FuncInfo->getFramePointerFI();

    // l.sw fp_ss(r1), r2
    TII.storeRegToStackSlot(MBB, MBBI, FPReg, true, FI, RC, TRI);

    // l.ori r2, r1, 0
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::ORI), FPReg).addReg(SPReg).addImm(0);
  }

  if (FuncInfo->hasBasePointerStackSlot()) {
    unsigned BPReg = TRI->getBaseRegister();
    int FI = FuncInfo->getBasePointerFI();

    // l.sw bp_ss(r1), r14
    TII.storeRegToStackSlot(MBB, MBBI, BPReg, true, FI, RC, TRI);
  }

  unsigned ScratchReg = MRI.createVirtualRegister(&OR1K::GPRRegClass);

  if (TRI->needsStackRealignment(MF)) {
    assert(hasFP(MF) && "Stack realignment without FP not supported");
    unsigned AlignLog = Log2_32(MFI->getMaxAlignment());

    // Materialize in rT the new stack pointer value.
    emitSPUpdate(MBB, MBBI, DL, TII, StackSize, ScratchReg, ScratchReg, false);

    // l.srl rT, rT, AlignLog
    // l.sll r1, rT, AlignLog
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::SRL_ri), ScratchReg)
      .addReg(ScratchReg).addImm(AlignLog);
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::SLL_ri), SPReg)
      .addReg(ScratchReg, RegState::Kill).addImm(AlignLog);
  } else {
    // Inplace update of the stack pointer value.
    emitSPUpdate(MBB, MBBI, DL, TII, StackSize, SPReg, ScratchReg, false);
  }

  if (FuncInfo->hasBasePointerStackSlot()) {
    unsigned BPReg = TRI->getBaseRegister();

    // l.ori r14, r1, 0
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::ORI), BPReg).addReg(SPReg).addImm(0);
  }
}

void OR1KFrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetMachine &TM = MF.getTarget();
  const TargetInstrInfo &TII = *TM.getInstrInfo();
  auto TRI = static_cast<const OR1KRegisterInfo *>(TM.getRegisterInfo());
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  DebugLoc DL = MBBI->getDebugLoc();

  unsigned StackSize = MFI->getStackSize();

  const unsigned SPReg = OR1K::R1;
  const TargetRegisterClass *RC = &OR1K::GPRRegClass;
  unsigned ScratchReg = MRI.createVirtualRegister(&OR1K::GPRRegClass);

  if (FuncInfo->hasFramePointerStackSlot()) {
    unsigned FPReg = TRI->getFrameRegister(MF);

    // l.ori r1, r2, 0
    BuildMI(MBB, MBBI, DL, TII.get(OR1K::ORI), SPReg).addReg(FPReg).addImm(0);
  } else {
    assert(!hasBP(MF) && "Unexpected BP without FP.");
    emitSPUpdate(MBB, MBBI, DL, TII, StackSize, SPReg, ScratchReg, true);
  }

  if (FuncInfo->hasBasePointerStackSlot()) {
    unsigned BPReg = TRI->getBaseRegister();
    int FI = FuncInfo->getBasePointerFI();

    // l.lwz r14, bp_ss(r1)
    TII.loadRegFromStackSlot(MBB, MBBI, BPReg, FI, RC, TRI);
  }

  if (FuncInfo->hasFramePointerStackSlot()) {
    unsigned FPReg = TRI->getFrameRegister(MF);
    int FI = FuncInfo->getFramePointerFI();

    // l.lwz r2, fp_ss(r1)
    TII.loadRegFromStackSlot(MBB, MBBI, FPReg, FI, RC, TRI);
  }

  if (FuncInfo->hasReturnAddressStackSlot()) {
    unsigned RAReg = TRI->getRARegister();
    int FI = FuncInfo->getReturnAddressFI();

    // l.lwz r9, ra_ss(r1)
    TII.loadRegFromStackSlot(MBB, MBBI, RAReg, FI, RC, TRI);
  }
}

void OR1KFrameLowering::processFunctionBeforeCalleeSavedScan(
    MachineFunction &MF, RegScavenger *RS) const {
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetMachine &TM = MF.getTarget();
  const OR1KSubtarget &ST = TM.getSubtarget<OR1KSubtarget>();
  auto TRI = static_cast<const OR1KRegisterInfo *>(TM.getRegisterInfo());
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  bool IsPIC = MF.getTarget().getRelocationModel() == Reloc::PIC_;

  int64_t StackOffset = 0;
  unsigned RegSize = OR1K::GPRRegClass.getSize();

  if (MFI->hasCalls() || !MRI.def_empty(TRI->getRARegister())) {
    MRI.isPhysRegUsed(TRI->getRARegister());
    StackOffset -= RegSize;
    int RAFI = MFI->CreateFixedObject(RegSize, StackOffset, true);
    FuncInfo->setReturnAddressFI(RAFI);
  }

  if (hasFP(MF)) {
    MRI.setPhysRegUsed(TRI->getFrameRegister(MF));
    StackOffset -= RegSize;
    int FPFI = MFI->CreateFixedObject(RegSize, StackOffset, true);
    FuncInfo->setFramePointerFI(FPFI);
  }

  if (hasBP(MF)) {
    MRI.setPhysRegUsed(TRI->getBaseRegister());
    StackOffset -= RegSize;
    int BPFI = MFI->CreateFixedObject(RegSize, StackOffset, true);
    FuncInfo->setBasePointerFI(BPFI);
  }

  if (StackOffset < 0 && FuncInfo->isVariadic() && ST.isNewABI()) {
    OR1KMachineFunctionInfo::VarArgsInfo &VAInfo = FuncInfo->getVarArgsInfo();
    int64_t RegSaveAreaOffset = MFI->getObjectOffset(VAInfo.RegSaveAreaFI);
    MFI->setObjectOffset(VAInfo.RegSaveAreaFI, RegSaveAreaOffset + StackOffset);
  }

  if (IsPIC) {
    unsigned GPReg = TRI->getGlobalBaseRegister();
    if (MFI->hasCalls() || !MRI.def_empty(GPReg))
      MRI.setPhysRegUsed(GPReg);
  }
}

bool OR1KFrameLowering::requiresCustomSpillRestore(
    const MachineFunction &MF, unsigned Reg,
    const OR1KRegisterInfo *TRI) const {
  return (hasFP(MF) && Reg == TRI->getFrameRegister(MF)) ||
         (hasBP(MF) && Reg == TRI->getBaseRegister()) ||
         Reg == TRI->getRARegister();
}

bool OR1KFrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    const std::vector<CalleeSavedInfo> &CSI,
    const TargetRegisterInfo *TRI) const {
  MachineFunction &MF = *MBB.getParent();
  const TargetInstrInfo &TII = *MF.getTarget().getInstrInfo();
  auto OR1KTRI = static_cast<const OR1KRegisterInfo *>(TRI);
  for (std::vector<CalleeSavedInfo>::const_iterator I = CSI.begin(),
                                                    E = CSI.end();
       I != E; ++I) {
    unsigned Reg = I->getReg();
    MBB.addLiveIn(Reg);

    if (requiresCustomSpillRestore(MF, Reg, OR1KTRI))
      continue;

    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.storeRegToStackSlot(MBB, MI, Reg, true, I->getFrameIdx(), RC, TRI);
  }
  return true;
}

bool OR1KFrameLowering::restoreCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    const std::vector<CalleeSavedInfo> &CSI,
    const TargetRegisterInfo *TRI) const {
  MachineFunction &MF = *MBB.getParent();
  const TargetInstrInfo &TII = *MF.getTarget().getInstrInfo();
  auto OR1KTRI = static_cast<const OR1KRegisterInfo *>(TRI);
  for (std::vector<CalleeSavedInfo>::const_reverse_iterator I = CSI.rbegin(),
                                                            E = CSI.rend();
       I != E; ++I) {
    unsigned Reg = I->getReg();
    if (!MBB.isLiveIn(Reg))
      MBB.addLiveIn(Reg);

    if (requiresCustomSpillRestore(MF, Reg, OR1KTRI))
      continue;

    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.loadRegFromStackSlot(MBB, MI, Reg, I->getFrameIdx(), RC, TRI);
  }
  return true;
}

void OR1KFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  assert(MI->getOpcode() == OR1K::ADJCALLSTACKDOWN ||
         MI->getOpcode() == OR1K::ADJCALLSTACKUP);

  int64_t Addend = MI->getOperand(0).getImm();

  if (!Addend || hasReservedCallFrame(MF)) {
    MBB.erase(MI);
    return;
  }

  const TargetMachine &TM = MF.getTarget();
  const TargetInstrInfo &TII = *TM.getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  const unsigned SPReg = OR1K::R1;
  DebugLoc dl = MI->getDebugLoc();

  if (MI->getOpcode() == OR1K::ADJCALLSTACKDOWN)
    Addend = -Addend;

  if (isInt<16>(Addend)) {
    // l.addi r1, r1, Addend
    BuildMI(MBB, MI, dl, TII.get(OR1K::ADDI), SPReg)
     .addReg(SPReg).addImm(Addend);
  } else {
    unsigned VReg = MRI.createVirtualRegister(&OR1K::GPRRegClass);

    // l.movhi rT, hi(Addend)
    // l.ori rT, rT, lo(Addend)
    // l.add r1, r1, rT
    BuildMI(MBB, MI, dl, TII.get(OR1K::MOVHI), VReg)
        .addImm((Addend >> 16) & 0xFFFFU);
    BuildMI(MBB, MI, dl, TII.get(OR1K::ORI), VReg)
     .addReg(VReg).addImm(Addend & 0xFFFFU);
    BuildMI(MBB, MI, dl, TII.get(OR1K::ADD), SPReg)
     .addReg(SPReg).addReg(VReg, RegState::Kill);
  }
  MBB.erase(MI);
}
