//===-- OR1KRegisterInfo.h - OR1K Register Information Impl -----*- C++ -*-===//
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

#ifndef OR1KREGISTERINFO_H
#define OR1KREGISTERINFO_H

#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "OR1KGenRegisterInfo.inc"

namespace llvm {
class TargetInstrInfo;
class Type;

class OR1KRegisterInfo : public OR1KGenRegisterInfo {
public:
  const TargetInstrInfo &TII;

  OR1KRegisterInfo(const TargetInstrInfo &tii);

  const uint16_t *getCalleeSavedRegs(const MachineFunction *MF = 0) const;

  BitVector getReservedRegs(const MachineFunction &MF) const;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override;
  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = NULL) const;

  bool needsStackRealignment(const MachineFunction &MF) const;

  bool hasReservedSpillSlot(const MachineFunction &MF, unsigned Reg,
                            int &FrameIndex) const override;

  bool hasReservedGlobalBaseRegister(const MachineFunction &MF) const;
  unsigned getGlobalBaseRegister() const;

  unsigned getFrameRegister(const MachineFunction &MF) const;

  unsigned getBaseRegister() const;
};
} // end namespace llvm

#endif
