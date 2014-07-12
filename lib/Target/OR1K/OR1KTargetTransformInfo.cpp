//===-- OR1KTargetTransformInfo.cpp - Define TargetMachine for OR1K -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a TargetTransformInfo analysis pass specific to the
// OR1K target machine. It uses the target's detailed information to provide
// more precise answers to certain TTI queries, while letting the target
// independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//


#include "OR1K.h"
#include "OR1KTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/CostTable.h"
#include "llvm/Target/TargetLowering.h"

#define DEBUG_TYPE "or1k-tti"

using namespace llvm;

// Declare the pass initialization routine locally as target-specific passes
// don't have a target-wide initialization entry point, and so we rely on the
// pass constructor initialization.
namespace llvm {
void initializeOR1KTTIPass(PassRegistry &);
}

namespace {
class OR1KTTI final : public ImmutablePass, public TargetTransformInfo {
  const OR1KSubtarget *ST;
  const OR1KTargetLowering *TLI;

public:
  OR1KTTI() : ImmutablePass(ID), ST(0), TLI(0) {
    llvm_unreachable("This pass cannot be directly constructed");
  }

  OR1KTTI(const OR1KTargetMachine *TM)
      : ImmutablePass(ID), ST(TM->getSubtargetImpl()),
        TLI(TM->getTargetLowering()) {
    initializeOR1KTTIPass(*PassRegistry::getPassRegistry());
  }

  void initializePass() override { pushTTIStack(this); }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    TargetTransformInfo::getAnalysisUsage(AU);
  }

  /// Pass identification.
  static char ID;

  /// Provide necessary pointer adjustments for the two base classes.
  void *getAdjustedAnalysisPointer(const void *ID) override {
    if (ID == &TargetTransformInfo::ID)
      return (TargetTransformInfo *)this;
    return this;
  }

  unsigned getIntImmCost(const APInt &Imm, Type *Ty) const override;

  unsigned getNumberOfRegisters(bool Vector) const override {
    // OR1K doesn't have vector register.
    if (Vector)
      return 0;

    // OR1K has 32 registers but R0, SP, FP, LR cannot be used as general
    // purpose registers.
    return 28;
  }

  unsigned getRegisterBitWidth(bool Vector) const override {
    // All the OR1K's register are 32 bit wide.
    return 32;
  }

  unsigned getMaximumUnrollFactor() const override {
    // OR1K is an in-order single issue CPU.
    return 1;
  }
};
} // end anonymous namespace

INITIALIZE_AG_PASS(OR1KTTI, TargetTransformInfo, "or1k-tti",
                   "OR1K Target Transform Info", true, true, false)
char OR1KTTI::ID = 0;

ImmutablePass *
llvm::createOR1KTargetTransformInfoPass(const OR1KTargetMachine *TM) {
  return new OR1KTTI(TM);
}

unsigned OR1KTTI::getIntImmCost(const APInt &Imm, Type *Ty) const {
  // 16-bits -> loaded with one l.ori instruction.
  // 32-bits -> loaded with onr l.ori for the lower 16 bits and
  //            one l.movhi for the higher 16 bits.
  // 64-bits -> loaded as two 32 bits immediates.

  assert(Ty->isIntegerTy());

  unsigned Bits = Ty->getPrimitiveSizeInBits();
  if (Bits == 16) {
    return 1;
  } else if (Bits == 32) {
    return 2;
  } else {
    return 4;
  }
}
