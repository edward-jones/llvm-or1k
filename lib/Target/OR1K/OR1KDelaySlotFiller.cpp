//===-- OR1kDelaySlotFiller.cpp - OR1K delay slot filler --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Simple pass to fills delay slots with useful instructions.
//
//===----------------------------------------------------------------------===//

#include "OR1K.h"
#include "OR1KTargetMachine.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetInstrInfo.h"

#define DEBUG_TYPE "or1k-delay-slot-filler"

using namespace llvm;

typedef OR1KSubtarget::DelayType DelayType;

STATISTIC(FilledSlots, "Number of delay slots filled");

namespace {
class Filler : public MachineFunctionPass {
public:
  /// \brief Target machine description which we query for reg. names, data
  /// layout, etc.
  TargetMachine &TM;
  const TargetInstrInfo *TII;
  MachineBasicBlock::instr_iterator LastFiller;

  static char ID;
  Filler(TargetMachine &tm)
      : MachineFunctionPass(ID), TM(tm), TII(tm.getInstrInfo()) {}

  const char *getPassName() const override {
    return "OR1K Delay Slot Filler";
  }

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) {
    bool Changed = false;
    for (MachineFunction::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI)
      Changed |= runOnMachineBasicBlock(*FI);
    return Changed;
  }

  void insertDefsUses(MachineBasicBlock::instr_iterator MI,
                      SmallSet<unsigned, 32> &RegDefs,
                      SmallSet<unsigned, 32> &RegUses,
                      SmallSet<unsigned, 32> &RegDeadDefs);

  bool IsRegInSet(SmallSet<unsigned, 32> &RegSet, unsigned Reg);

  bool delayHasHazard(MachineBasicBlock::instr_iterator MI, bool &sawLoad,
                      bool &sawStore, SmallSet<unsigned, 32> &RegDefs,
                      SmallSet<unsigned, 32> &RegUses,
                      SmallSet<unsigned, 32> &RegDeadDefs);

  bool findDelayInstr(MachineBasicBlock &MBB,
                      MachineBasicBlock::instr_iterator slot,
                      MachineBasicBlock::instr_iterator &Filler);
};
char Filler::ID = 0;
} // end of anonymous namespace

/// \brief Returns a pass that fills in delay slots in OR1K MachineFunctions.
FunctionPass *llvm::createOR1KDelaySlotFillerPass(OR1KTargetMachine &tm) {
  return new Filler(tm);
}

/// \brief Fill in delay slots for the given basic block.
/// There is only one delay slot per delayed instruction.
bool Filler::runOnMachineBasicBlock(MachineBasicBlock &MBB) {
  bool Changed = false;
  LastFiller = MBB.instr_end();
  DelayType Delay = TM.getSubtarget<OR1KSubtarget>().delaySlotType();

  // No delay slots
  if (Delay == DelayType::NoDelay)
    return false;

  for (MachineBasicBlock::instr_iterator I = MBB.instr_begin();
       I != MBB.instr_end(); ++I)
    if (I->getDesc().hasDelaySlot()) {
      MachineBasicBlock::instr_iterator J = I;

      if (TM.getOptLevel() != CodeGenOpt::None &&
          !(Delay == DelayType::CompatDelay) && findDelayInstr(MBB, I, J))
        MBB.splice(std::next(I), &MBB, J);
      else
        BuildMI(MBB, std::next(I), DebugLoc(), TII->get(OR1K::NOP)).addImm(0);

      ++FilledSlots;
      Changed = true;
      // Record the filler instruction that filled the delay slot.
      // The instruction after it will be visited in the next iteration.
      LastFiller = ++I;

      // Set InsideBundle bit so that the machine verifier doesn't expect this
      // instruction to be a terminator.
      LastFiller->bundleWithPred();
    }
  return Changed;
}

bool Filler::findDelayInstr(MachineBasicBlock &MBB,
                            MachineBasicBlock::instr_iterator slot,
                            MachineBasicBlock::instr_iterator &Filler) {

  SmallSet<unsigned, 32> RegDefs;
  SmallSet<unsigned, 32> RegUses;
  SmallSet<unsigned, 32> RegDeadDefs;

  insertDefsUses(slot, RegDefs, RegUses, RegDeadDefs);

  bool sawLoad = false;
  bool sawStore = false;

  for (MachineBasicBlock::reverse_instr_iterator I(slot); I != MBB.instr_rend();
       ++I) {
    // skip debug value
    if (I->isDebugValue())
      continue;

    // Convert to forward iterator.
    MachineBasicBlock::instr_iterator FI(std::next(I).base());

    if (I->hasUnmodeledSideEffects() || I->isInlineAsm() || I->isLabel() ||
        FI == LastFiller || I->isPseudo())
      break;

    if (delayHasHazard(FI, sawLoad, sawStore, RegDefs, RegUses, RegDeadDefs)) {
      insertDefsUses(FI, RegDefs, RegUses, RegDeadDefs);
      continue;
    }
    Filler = FI;
    return true;
  }
  return false;
}

bool Filler::delayHasHazard(MachineBasicBlock::instr_iterator MI, bool &sawLoad,
                            bool &sawStore, SmallSet<unsigned, 32> &RegDefs,
                            SmallSet<unsigned, 32> &RegUses,
                            SmallSet<unsigned, 32> &RegDeadDefs) {
  if (MI->isImplicitDef() || MI->isKill())
    return true;

  // Loads or stores cannot be moved past a store to the delay slot
  // and stores cannot be moved past a load.
  if (MI->mayLoad()) {
    if (sawStore)
      return true;
    sawLoad = true;
  }

  if (MI->mayStore()) {
    if (sawStore)
      return true;
    sawStore = true;
    if (sawLoad)
      return true;
  }

  assert((!MI->isCall() && !MI->isReturn()) &&
         "Cannot put calls or returns in delay slot.");

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    unsigned Reg;

    if (!MO.isReg() || !(Reg = MO.getReg()))
      continue; // skip

    if (MO.isDef()) {
      // check whether Reg is defined or used before delay slot.
      if (IsRegInSet(RegDefs, Reg) || IsRegInSet(RegUses, Reg))
        return true;
    }
    if (MO.isUse()) {
      // check whether Reg is defined before delay slot.
      if (IsRegInSet(RegDefs, Reg) || IsRegInSet(RegDeadDefs, Reg))
        return true;
    }
  }
  return false;
}

/// \brief Insert Defs and Uses of MI into the sets RegDefs and RegUses.
void Filler::insertDefsUses(MachineBasicBlock::instr_iterator MI,
                            SmallSet<unsigned, 32> &RegDefs,
                            SmallSet<unsigned, 32> &RegUses,
                            SmallSet<unsigned, 32> &RegDeadDefs) {
  // If MI is a call or return, just examine the explicit non-variadic operands.
  MCInstrDesc MCID = MI->getDesc();
  unsigned e = MI->isCall() || MI->isReturn() ? MCID.getNumOperands()
                                              : MI->getNumOperands();

  // Add R9 to RegDefs to prevent users of R9 from going into delay slot.
  if (MI->isCall())
    RegDefs.insert(OR1K::R9);

  // Add R9 to RegUses to prevent definers of R9 from going into delay slot.
  if (MI->isReturn())
    RegUses.insert(OR1K::R9);

  for (unsigned i = 0; i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    unsigned Reg;

    if (!MO.isReg() || !(Reg = MO.getReg()))
      continue;

    if (MO.isDef() && !MO.isDead())
      RegDefs.insert(Reg);
    else if (MO.isDef() && MO.isDead())
      RegDeadDefs.insert(Reg);
    else if (MO.isUse())
      RegUses.insert(Reg);
  }
}

/// \brief Returns true if the Reg or its alias is in the RegSet.
bool Filler::IsRegInSet(SmallSet<unsigned, 32> &RegSet, unsigned Reg) {
  // Check Reg and all aliased Registers.
  for (MCRegAliasIterator AI(Reg, TM.getRegisterInfo(), true); AI.isValid();
       ++AI)
    if (RegSet.count(*AI))
      return true;
  return false;
}
