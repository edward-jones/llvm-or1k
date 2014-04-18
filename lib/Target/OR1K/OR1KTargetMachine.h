//===-- OR1KTargetMachine.h - Define TargetMachine for OR1K --- C++ ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the OR1K specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef OR1K_TARGETMACHINE_H
#define OR1K_TARGETMACHINE_H

#include "OR1KSubtarget.h"
#include "OR1KInstrInfo.h"
#include "OR1KISelLowering.h"
#include "OR1KSelectionDAGInfo.h"
#include "OR1KFrameLowering.h"
#include "llvm/PassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/IR/DataLayout.h"

namespace llvm {
  class OR1KTargetMachine : public LLVMTargetMachine {
    OR1KSubtarget Subtarget;
    const DataLayout DL;
    OR1KInstrInfo InstrInfo;
    OR1KTargetLowering TLInfo;
    OR1KSelectionDAGInfo TSInfo;
    OR1KFrameLowering FrameLowering;

  public:
    OR1KTargetMachine(const Target &T, StringRef TT,
                        StringRef CPU, StringRef FS,
                        const TargetOptions &Options,
                        Reloc::Model RM, CodeModel::Model CM,
                        CodeGenOpt::Level OL);

    const OR1KInstrInfo *getInstrInfo() const override {
      return &InstrInfo;
    }

    const TargetFrameLowering *getFrameLowering() const override {
      return &FrameLowering;
    }

    const OR1KSubtarget *getSubtargetImpl() const override {
      return &Subtarget;
    }

    const DataLayout *getDataLayout() const override {
      return &DL;
    }

    const OR1KRegisterInfo *getRegisterInfo() const override {
      return &InstrInfo.getRegisterInfo();
    }

    const OR1KTargetLowering *getTargetLowering() const override {
     return &TLInfo;
    }

    const OR1KSelectionDAGInfo* getSelectionDAGInfo() const override{
      return &TSInfo;
    }

    TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

    void addAnalysisPasses(PassManagerBase &PM) override {
      PM.add(createBasicTargetTransformInfoPass(this));
      PM.add(createOR1KTargetTransformInfoPass(this));
    }
  };
} // End llvm namespace

#endif
