//===-- OR1KISelLowering.h - OR1K DAG Lowering Interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that OR1K uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_OR1K_ISELLOWERING_H
#define LLVM_TARGET_OR1K_ISELLOWERING_H

#include "OR1K.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetLowering.h"

namespace llvm {
namespace OR1KISD {
enum {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  Return,
  Call,
  Select,
  SetFlag,
  BrCond,
  FF1,
  FL1,
  HiLo
};
}

class OR1KSubtarget;
class OR1KTargetMachine;

class OR1KTargetLowering : public TargetLowering {
public:
  explicit OR1KTargetLowering(OR1KTargetMachine &TM);

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  const char *getTargetNodeName(unsigned Opcode) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               SDLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool isVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins, SDLoc dl,
                          SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, SDLoc dl,
                      SelectionDAG &DAG) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const std::string &Constraint,
                               MVT VT) const override;
  ConstraintWeight
  getSingleConstraintMatchWeight(AsmOperandInfo &info,
                                 const char *constraint) const override;

  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr *MI,
                              MachineBasicBlock *BB) const override;

  bool isLegalAddImmediate(int64_t) const override;
  bool isLegalICmpImmediate(int64_t) const override;
  bool isLegalAddressingMode(const AddrMode &AM, Type *Ty) const override;

private:
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVACOPY(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCTTZ(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCTLZ(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCTTZ_ZERO_UNDEF(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;

private:
  SDValue getSetFlag(SDLoc dl, SDValue LHS, SDValue RHS, ISD::CondCode CC,
                     bool &Negate, SelectionDAG &DAG) const;

private:
  const OR1KSubtarget &Subtarget;
  const OR1KTargetMachine &TM;
  const DataLayout *DL;
};
}

#endif
