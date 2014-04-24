//===- OR1KMachineFuctionInfo.h - OR1K machine func info ---------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares OR1K-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef OR1KMACHINEFUNCTIONINFO_H
#define OR1KMACHINEFUNCTIONINFO_H
#include "OR1KRegisterInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm {

class OR1KMachineFunctionInfo : public MachineFunctionInfo {

  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex;

public:
  OR1KMachineFunctionInfo(MachineFunction &MF)
   : SRetReg(0), GlobalBaseReg(0),
     ReturnAddressFI(0), FramePointerFI(0), BasePointerFI(0) {}

  bool hasSRetReturnReg() const { return SRetReg != 0; }
  unsigned getSRetReturnReg() const { return SRetReg; }
  void setSRetReturnReg(unsigned Reg) { SRetReg = Reg; }

  unsigned getGlobalBaseReg() const { return GlobalBaseReg; }
  void setGlobalBaseReg(unsigned Reg) { GlobalBaseReg = Reg; }

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

  bool hasReturnAddressStackSlot() const { return ReturnAddressFI < 0; }
  int getReturnAddressFI() const { return ReturnAddressFI; }
  void setReturnAddressFI(int FI) { ReturnAddressFI = FI; }

  bool hasFramePointerStackSlot() const { return FramePointerFI < 0; }
  int getFramePointerFI() const { return FramePointerFI; }
  void setFramePointerFI(int FI) { FramePointerFI = FI; }

  bool hasBasePointerStackSlot() const { return BasePointerFI < 0; }
  int getBasePointerFI() const { return BasePointerFI; }
  void setBasePointerFI(int FI) { BasePointerFI = FI; }

  bool isFrameIndexUsingPreviousSP(int FI) const {
    return (hasReturnAddressStackSlot() && FI == ReturnAddressFI) ||
           (hasFramePointerStackSlot() && FI == FramePointerFI) ||
           (hasBasePointerStackSlot() && FI == BasePointerFI);
  }

private:
  unsigned SRetReg;
  unsigned GlobalBaseReg;
  int ReturnAddressFI;
  int FramePointerFI;
  int BasePointerFI;
};

} // End llvm namespace

#endif
