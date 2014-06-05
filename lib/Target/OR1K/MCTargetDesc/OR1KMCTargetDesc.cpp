//===-- OR1KMCTargetDesc.cpp - OR1K Target Descriptions -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides OR1K specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "OR1KMCTargetDesc.h"
#include "OR1KMCAsmInfo.h"
#include "InstPrinter/OR1KInstPrinter.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "OR1KGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "OR1KGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "OR1KGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createOR1KMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitOR1KMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createOR1KMCRegisterInfo(StringRef TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitOR1KMCRegisterInfo(X, OR1K::R9);
  return X;
}

static MCSubtargetInfo *createOR1KMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                  StringRef FS) {
  MCSubtargetInfo *X = new MCSubtargetInfo();
  InitOR1KMCSubtargetInfo(X, TT, CPU, FS);
  return X;
}

static MCCodeGenInfo *createOR1KMCCodeGenInfo(StringRef TT, Reloc::Model RM,
                                              CodeModel::Model CM,
                                              CodeGenOpt::Level OL) {
  MCCodeGenInfo *X = new MCCodeGenInfo();
  X->InitMCCodeGenInfo(RM, CM, OL);
  return X;
}

static MCStreamer *
createOR1KMCStreamer(const Target &T, StringRef TT, MCContext &Ctx,
                     MCAsmBackend &MAB, raw_ostream &OS, MCCodeEmitter *Emitter,
                     const MCSubtargetInfo &STI, bool RelaxAll,
                     bool NoExecStack) {
  return createELFStreamer(Ctx, MAB, OS, Emitter, RelaxAll, NoExecStack);
}

static MCStreamer *
createOR1KMCAsmStreamer(MCContext &Ctx, formatted_raw_ostream &OS,
                        bool isVerboseAsm, bool useDwarfDirectory,
                        MCInstPrinter *InstPrint, MCCodeEmitter *CE,
                        MCAsmBackend *TAB, bool ShowInst) {
  return llvm::createAsmStreamer(Ctx, OS, isVerboseAsm, useDwarfDirectory,
                                 InstPrint, CE, TAB, ShowInst);
}


static MCInstPrinter *createOR1KMCInstPrinter(const Target &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI,
                                              const MCSubtargetInfo &STI) {
  return (SyntaxVariant == 0) ? new OR1KInstPrinter(MAI, MII, MRI) : nullptr;
}

extern "C" void LLVMInitializeOR1KTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfo<OR1KMCAsmInfo> X(TheOR1KbeTarget);
  RegisterMCAsmInfo<OR1KMCAsmInfo> Y(TheOR1KleTarget);

  typedef TargetRegistry TR;

  // Register the MC codegen info.
  TR::RegisterMCCodeGenInfo(TheOR1KbeTarget, createOR1KMCCodeGenInfo);
  TR::RegisterMCCodeGenInfo(TheOR1KleTarget, createOR1KMCCodeGenInfo);

  // Register the MC instruction info.
  TR::RegisterMCInstrInfo(TheOR1KbeTarget, createOR1KMCInstrInfo);
  TR::RegisterMCInstrInfo(TheOR1KleTarget, createOR1KMCInstrInfo);

  // Register the MC register info.
  TR::RegisterMCRegInfo(TheOR1KbeTarget, createOR1KMCRegisterInfo);
  TR::RegisterMCRegInfo(TheOR1KleTarget, createOR1KMCRegisterInfo);

  // Register the MC subtarget info.
  TR::RegisterMCSubtargetInfo(TheOR1KbeTarget, createOR1KMCSubtargetInfo);
  TR::RegisterMCSubtargetInfo(TheOR1KleTarget, createOR1KMCSubtargetInfo);

  // Register the MC code emitter.
  TR::RegisterMCCodeEmitter(TheOR1KbeTarget, llvm::createOR1KbeMCCodeEmitter);
  TR::RegisterMCCodeEmitter(TheOR1KleTarget, llvm::createOR1KleMCCodeEmitter);

  // Register the ASM Backend.
  TR::RegisterMCAsmBackend(TheOR1KbeTarget, createOR1KbeAsmBackend);
  TR::RegisterMCAsmBackend(TheOR1KleTarget, createOR1KleAsmBackend);

  // Register the asm streamer.
  TR::RegisterAsmStreamer(TheOR1KbeTarget, createOR1KMCAsmStreamer);
  TR::RegisterAsmStreamer(TheOR1KleTarget, createOR1KMCAsmStreamer);

  // Register the object streamer.
  TR::RegisterMCObjectStreamer(TheOR1KbeTarget, createOR1KMCStreamer);
  TR::RegisterMCObjectStreamer(TheOR1KleTarget, createOR1KMCStreamer);

  // Register the MCInstPrinter.
  TR::RegisterMCInstPrinter(TheOR1KbeTarget, createOR1KMCInstPrinter);
  TR::RegisterMCInstPrinter(TheOR1KleTarget, createOR1KMCInstPrinter);
}
