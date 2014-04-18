//===-- OR1KTargetMachine.cpp - Define TargetMachine for OR1K ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about OR1K target spec.
//
//===----------------------------------------------------------------------===//

#include "OR1K.h"
#include "OR1KTargetMachine.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

extern "C" void LLVMInitializeOR1KTarget() {
  // Register the target.
  RegisterTargetMachine<OR1KTargetMachine> X(TheOR1KTarget);
}

// DataLayout --> Big-endian, 32-bit pointer/ABI/alignment
// The stack is always 4 byte aligned
// On function prologue, the stack is created by decrementing
// its pointer. Once decremented, all references are done with positive
// offset from the stack/frame pointer.
OR1KTargetMachine::
OR1KTargetMachine(const Target &T, StringRef TT,
                    StringRef CPU, StringRef FS, const TargetOptions &Options,
                    Reloc::Model RM, CodeModel::Model CM,
                    CodeGenOpt::Level OL)
  : LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
  Subtarget(TT, CPU, FS),
  DL("E-m:e-p:32:32-i64:32-f64:32-v64:32-v128:32-a:0:32-n32"),
  InstrInfo(*this), TLInfo(*this), TSInfo(*this),
  FrameLowering(Subtarget), InstrItins(Subtarget.getInstrItineraryData()) {
  initAsmInfo();
}

namespace {
/// OR1K Code Generator Pass Configuration Options.
class OR1KPassConfig : public TargetPassConfig {
public:
  OR1KPassConfig(OR1KTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  OR1KTargetMachine &getOR1KTargetMachine() const {
    return getTM<OR1KTargetMachine>();
  }

  void addIRPasses() override;

  bool addInstSelector() override;
  bool addPreEmitPass() override;
  bool addPreISel() override;
};
} // namespace

TargetPassConfig *OR1KTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new OR1KPassConfig(this, PM);
}

// Install an instruction selector pass using
// the ISelDag to gen OR1K code.
bool OR1KPassConfig::addInstSelector() {
  addPass(createOR1KISelDag(getOR1KTargetMachine()));

  return false;
}

// Implemented by targets that want to run passes immediately before
// machine code is emitted. return true if -print-machineinstrs should
// print out the code after the passes.

bool OR1KPassConfig::addPreEmitPass() {
  addPass(createOR1KDelaySlotFillerPass(getOR1KTargetMachine()));
  addPass(createOR1KFunnyNOPReplacer());
  return true;
}

bool OR1KPassConfig::addPreISel() {
  return true;
}

/// Add common target configurable passes that perform LLVM IR to IR transforms
/// following machine independent optimization.
void OR1KPassConfig::addIRPasses() {
  // Basic AliasAnalysis support.
  // Add TypeBasedAliasAnalysis before BasicAliasAnalysis so that
  // BasicAliasAnalysis wins if they disagree. This is intended to help
  // support "obvious" type-punning idioms.
  addPass(createTypeBasedAliasAnalysisPass());
  addPass(createBasicAliasAnalysisPass());

  // Run loop strength reduction before anything else.
  if (getOptLevel() != CodeGenOpt::None) {
    addPass(createOR1KLoopStrengthReduction());
  }

  addPass(createGCLoweringPass());

  // Make sure that no unreachable blocks are instruction selected.
  addPass(createUnreachableBlockEliminationPass());

  // Prepare expensive constants for SelectionDAG.
  if (getOptLevel() != CodeGenOpt::None)
    addPass(createConstantHoistingPass());
}
