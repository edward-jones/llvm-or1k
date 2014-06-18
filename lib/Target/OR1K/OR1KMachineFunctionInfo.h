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
public:
  struct VarArgsInfo {
    unsigned AllocatedGPR;
    int StackAreaFI;
    int RegSaveAreaFI;
    int VarArgsAreaFI;

    VarArgsInfo() : AllocatedGPR(0), StackAreaFI(0), RegSaveAreaFI(0) {}
  };

public:
  OR1KMachineFunctionInfo(MachineFunction &MF)
   : SRetReg(0), GlobalBaseReg(0), RegSaveAreaSize(0), ReturnAddressFI(0),
     FramePointerFI(0), BasePointerFI(0), IsVariadic(false) {}

  bool hasSRetReturnReg() const { return SRetReg != 0; }
  unsigned getSRetReturnReg() const { return SRetReg; }
  void setSRetReturnReg(unsigned Reg) { SRetReg = Reg; }

  unsigned getGlobalBaseReg() const { return GlobalBaseReg; }
  void setGlobalBaseReg(unsigned Reg) { GlobalBaseReg = Reg; }

  unsigned getRegSaveAreaSize() const { return RegSaveAreaSize; }
  void setRegSaveAreaSize(unsigned Size) { RegSaveAreaSize = Size; }

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

  bool isVariadic() const { return IsVariadic; }
  void setVariadic(bool VA) { IsVariadic = VA; }

  const VarArgsInfo &getVarArgsInfo() const {
    assert(IsVariadic);
    return VAInfo;
  }

  VarArgsInfo &getVarArgsInfo() {
    assert(IsVariadic);
    return VAInfo;
  }

private:
  unsigned SRetReg;
  unsigned GlobalBaseReg;
  unsigned RegSaveAreaSize;
  int ReturnAddressFI;
  int FramePointerFI;
  int BasePointerFI;
  bool IsVariadic;
  VarArgsInfo VAInfo;
};

} // End llvm namespace

#endif
