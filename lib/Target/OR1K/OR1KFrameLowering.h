//===-- OR1KFrameLowering.h - Define frame lowering for OR1K ---*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef OR1K_FRAMEINFO_H
#define OR1K_FRAMEINFO_H

#include "OR1K.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {
class OR1KRegisterInfo;
class OR1KSubtarget;

class OR1KFrameLowering : public TargetFrameLowering {
public:
  explicit OR1KFrameLowering(const OR1KSubtarget &sti)
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 4, 0, 4),
        STI(sti) {}

  bool hasFP(const MachineFunction &MF) const;

  bool hasBP(const MachineFunction &MF) const;

  bool hasReservedCallFrame(const MachineFunction &MF) const override;

  void emitPrologue(MachineFunction &MF) const override;

  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  void processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                            RegScavenger *RS) const override;

  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const override;

  bool
  restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
                              const std::vector<CalleeSavedInfo> &CSI,
                              const TargetRegisterInfo *TRI) const override;

  void
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;

private:
  void emitSPUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
                    DebugLoc DL, const TargetInstrInfo &TII, unsigned StackSize,
                    unsigned DestReg, unsigned TmpReg, bool OnEpilogue) const;

  bool requiresCustomSpillRestore(const MachineFunction &MF, unsigned Reg,
                                  const OR1KRegisterInfo *TRI) const;

protected:
  const OR1KSubtarget &STI;
};
} // end llvm namespace

#endif
