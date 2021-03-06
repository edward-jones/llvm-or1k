//===-- OR1KInstrInfo.cpp - OR1K Instruction Information --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the OR1K implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//


#include "OR1K.h"
#include "OR1KInstrInfo.h"
#include "OR1KMachineFunctionInfo.h"
#include "OR1KSubtarget.h"
#include "OR1KTargetMachine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define DEBUG_TYPE "or1k-instruction-info"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "OR1KGenInstrInfo.inc"

OR1KInstrInfo::OR1KInstrInfo(TargetMachine &TM)
    : OR1KGenInstrInfo(OR1K::ADJCALLSTACKDOWN, OR1K::ADJCALLSTACKUP),
      RI(*this) {}

void OR1KInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I, DebugLoc DL,
                                unsigned DestReg, unsigned SrcReg,
                                bool KillSrc) const {
  if (OR1K::GPRRegClass.contains(DestReg, SrcReg))
    BuildMI(MBB, I, DL, get(OR1K::ORI), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addImm(0);
  else if (OR1K::SPRRegClass.contains(SrcReg, DestReg))
    llvm_unreachable("Impossible reg-to-reg copy with special register as"
                     "source and destination");
  else if (OR1K::SPRRegClass.contains(SrcReg))
    BuildMI(MBB, I, DL, get(OR1K::MFSPR), DestReg)
        .addReg(OR1K::R0)
        .addImm(RI.getEncodingValue(SrcReg))
        .addReg(SrcReg, KillSrc ? RegState::ImplicitKill : RegState::Implicit);
  else if (OR1K::SPRRegClass.contains(DestReg))
    BuildMI(MBB, I, DL, get(OR1K::MTSPR), SrcReg)
        .addReg(OR1K::R0)
        .addImm(RI.getEncodingValue(DestReg))
        .addReg(DestReg, RegState::ImplicitDefine);
}

void OR1KInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator I,
                                        unsigned SrcReg, bool isKill, int FI,
                                        const TargetRegisterClass *RC,
                                        const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  if (RC == &OR1K::GPRRegClass)
    BuildMI(MBB, I, DL, get(OR1K::SW))
        .addReg(SrcReg, getKillRegState(isKill))
        .addFrameIndex(FI)
        .addImm(0);
  else
    llvm_unreachable("Can't store this register to stack slot");
}

void OR1KInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator I,
                                         unsigned DestReg, int FI,
                                         const TargetRegisterClass *RC,
                                         const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  if (RC == &OR1K::GPRRegClass)
    BuildMI(MBB, I, DL, get(OR1K::LWZ), DestReg).addFrameIndex(FI).addImm(0);
  else
    llvm_unreachable("Can't load this register from stack slot");
}

bool OR1KInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,
                                  MachineBasicBlock *&TBB,
                                  MachineBasicBlock *&FBB,
                                  SmallVectorImpl<MachineOperand> &Cond,
                                  bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue())
      continue;

    // Working from the bottom, when we see a non-terminator
    // instruction, we're done.
    if (!isUnpredicatedTerminator(I))
      break;

    // A terminator that isn't a branch can't easily be handled
    // by this analysis.
    if (!I->isBranch())
      return true;

    // Cannot handle indirect branches.
    if (I->getOpcode() == OR1K::JR)
      return true;

    // Handle unconditional branches.
    if (I->getOpcode() == OR1K::J) {
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a J, delete them.
      while (std::next(I) != MBB.end())
        std::next(I)->eraseFromParent();
      Cond.clear();
      FBB = 0;

      // Delete the J if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = 0;
        I->eraseFromParent();
        I = MBB.end();
        continue;
      }

      // TBB is used to indicate the unconditinal destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }
    // Cannot handle conditional branches
    return true;
  }

  return false;
}

unsigned OR1KInstrInfo::InsertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    const SmallVectorImpl<MachineOperand> &Cond, DebugLoc DL) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");

  if (Cond.empty()) {
    // Unconditional branch?
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(OR1K::J)).addMBB(TBB);
    return 1;
  }

  llvm_unreachable("Unexpected conditional branch");
  return 0;
}

unsigned OR1KInstrInfo::RemoveBranch(MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue())
      continue;
    if (I->getOpcode() != OR1K::J && I->getOpcode() != OR1K::JR)
      break;
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

unsigned OR1KInstrInfo::getGlobalBaseReg(MachineFunction &MF) const {
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  unsigned GlobalBaseReg = FuncInfo->getGlobalBaseReg();
  if (GlobalBaseReg)
    return GlobalBaseReg;

  // Insert the set of GlobalBaseReg into the first MBB of the function
  MachineBasicBlock &FirstMBB = MF.front();
  MachineBasicBlock::iterator MBBI = FirstMBB.begin();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();

  if (RI.hasReservedGlobalBaseRegister(MF))
    GlobalBaseReg = RI.getGlobalBaseRegister();
  else
    GlobalBaseReg = RegInfo.createVirtualRegister(&OR1K::GPRRegClass);

  FuncInfo->setGlobalBaseReg(GlobalBaseReg);

  DebugLoc DL;
  BuildMI(FirstMBB, MBBI, DL, get(OR1K::GET_GLOBAL_BASE), GlobalBaseReg);
  return GlobalBaseReg;
}
