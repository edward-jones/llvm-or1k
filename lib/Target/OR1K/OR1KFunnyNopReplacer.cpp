//===-- OR1kFunnyNOPReplacer.cpp - OR1K Funny Nop Replacer  ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implents the Funny NOP Replacer Pass. This pass is responsible for
// substituing normal NOPs with funny NOPs.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "funny-nop-replacer"

#include "OR1K.h"
#include "OR1KTargetMachine.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Process.h"

using namespace llvm;

static cl::opt<bool> FunnyNOPReplacer(
  "funny-or1k-nop-replacer",
  cl::init(false),
  cl::desc("Replace normal nops with funny nops"),
  cl::Hidden);

static unsigned int hexSpeakCodes[] = {
  0x10CC,
  0xB00B,
  0xB00C,
  0xB105,
  0xBAAD,
  0xBABE,
  0xBEAF,
  0xBEEF,
  0xC0DE,
  0xCAFE,
  0xD00D,
  0xF00D,
  0xFA11,
  0xFACE,
  0xFEE1,
  0xFEED
};

template <size_t N>
static unsigned int getRandomHexSpeakCode(const unsigned (&array)[N]) {
  return array[sys::Process::GetRandomNumber() % N];
}

namespace {
  class FunnyNOP : public MachineFunctionPass {
  public:
    /// Target machine description which we query for reg. names, data
    /// layout, etc.
    TargetMachine &TM;
    const TargetInstrInfo *TII;
    MachineBasicBlock::instr_iterator LastFiller;

    static char ID;
    FunnyNOP(TargetMachine &tm)
      : MachineFunctionPass(ID), TM(tm), TII(tm.getInstrInfo()) { }

    virtual const char *getPassName() const {
      return "OR1K Funny NOP replacer";
    }

    bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
    bool runOnMachineFunction(MachineFunction &F) {
      bool Changed = false;
      for (MachineFunction::iterator FI = F.begin(), FE = F.end();
           FI != FE; ++FI)
        Changed |= runOnMachineBasicBlock(*FI);
      return Changed;
    }
  };
  char FunnyNOP::ID = 0;
} // end of anonymous namespace

/// createOR1KFunnyNOPReplacer - Returns a pass that replaces normal NOPs with
/// funny NOPs
FunctionPass *llvm::createOR1KFunnyNOPReplacer(OR1KTargetMachine &tm) {
  return new FunnyNOP(tm);
}

/// runOnMachineBasicBlock - Fill in delay slots for the given basic block.
/// There is only one delay slot per delayed instruction.
bool FunnyNOP::runOnMachineBasicBlock(MachineBasicBlock &MBB) {
  bool Changed = false;

  // Replace NOPs only if enabled
  if (!FunnyNOPReplacer)
    return false;

  // Iterate over each instruction of the basic block
  for (MachineBasicBlock::instr_iterator I = MBB.instr_begin();
          I != MBB.instr_end(); ++I) {
    // If the instruction is a NOP replace its immediate value with a funny one.
    if(I->getDesc().getOpcode() == OR1K::NOP) {
      MachineOperand &NopImmediate = I->getOperand(0);
      NopImmediate.setImm(getRandomHexSpeakCode(hexSpeakCodes));
      Changed = true;
    }
  }
  return Changed;
}
