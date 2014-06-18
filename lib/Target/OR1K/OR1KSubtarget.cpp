//===- OR1KSubtarget.cpp - OR1K Subtarget Information -------------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the OR1K specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "OR1K.h"
#include "OR1KRegisterInfo.h"
#include "OR1KSubtarget.h"

#define DEBUG_TYPE "or1k-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "OR1KGenSubtargetInfo.inc"

using namespace llvm;

void OR1KSubtarget::anchor() { }

OR1KSubtarget::OR1KSubtarget(const std::string &TT, const std::string &CPU,
                             const std::string &FS, bool LittleEndian)
 : OR1KGenSubtargetInfo(TT, CPU, FS), OR1KABI(DefaultABI),
   HasMul(false), HasMul64(false), HasDiv(false), HasRor(false), HasCmov(false),
   HasMAC(false), HasExt(false), HasSFII(false), HasFBit(false),
   DelaySlotType(DelayType::Delay), IsLittleEndian(LittleEndian) {
  std::string CPUName = CPU;
  if (CPUName.empty())
    CPUName = "generic";

  ParseSubtargetFeatures(CPUName, FS);

  InstrItins = getInstrItineraryForCPU(CPUName);
}

bool
OR1KSubtarget::enablePostRAScheduler(CodeGenOpt::Level OptLevel,
                                     AntiDepBreakMode& Mode,
                                     RegClassVector& CriticalPathRCs) const {
  Mode = TargetSubtargetInfo::ANTIDEP_NONE;

  CriticalPathRCs.clear();
  CriticalPathRCs.push_back(&OR1K::GPRRegClass);

  return OptLevel >= CodeGenOpt::Default;
}
