//===-- OR1kFunnyNOPReplacer.cpp - OR1K Funny Nop Replacer  ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Funny NOP Replacer Pass. This pass is responsible
// for substituting normal NOPs with funny NOPs.
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
  static char ID;
  FunnyNOP() : MachineFunctionPass(ID) {}

  const char *getPassName() const override {
    return "OR1K Funny NOP replacer";
  }

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override {
    bool Changed = false;
    for (MachineBasicBlock &FB : F) {
      Changed |= runOnMachineBasicBlock(FB);
    }
    return Changed;
  }
};

} // end of anonymous namespace

char FunnyNOP::ID = 0;

/// createOR1KFunnyNOPReplacer - Returns a pass that replaces normal NOPs with
/// funny NOPs
FunctionPass *llvm::createOR1KFunnyNOPReplacer() {
  return new FunnyNOP();
}

/// runOnMachineBasicBlock - Fill in delay slots for the given basic block.
/// There is only one delay slot per delayed instruction.
bool FunnyNOP::runOnMachineBasicBlock(MachineBasicBlock &MBB) {
  bool Changed = false;

  // Replace NOPs only if enabled
  if (!FunnyNOPReplacer)
    return false;

  // Iterate over each instruction of the basic block
  for (MachineInstr &I : MBB) {
    // If the instruction is a NOP replace its immediate value with a funny one.
    if (I.getDesc().getOpcode() == OR1K::NOP) {
      MachineOperand &NopImmediate = I.getOperand(0);
      NopImmediate.setImm(getRandomHexSpeakCode(hexSpeakCodes));
      Changed = true;
    }
  }

  return Changed;
}
