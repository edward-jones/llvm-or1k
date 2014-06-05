//===-- OR1KISelLowering.cpp - OR1K DAG Lowering Implementation  ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the OR1KTargetLowering class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "or1k-lower"

#include "OR1KISelLowering.h"
#include "OR1K.h"
#include "OR1KMachineFunctionInfo.h"
#include "OR1KRegisterInfo.h"
#include "OR1KTargetMachine.h"
#include "OR1KSubtarget.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLowering.h"
using namespace llvm;

OR1KTargetLowering::OR1KTargetLowering(OR1KTargetMachine &tm) :
  TargetLowering(tm, new TargetLoweringObjectFileELF()),
  Subtarget(*tm.getSubtargetImpl()), TM(tm) {

  DL = getDataLayout();

  // Set up the register classes.
  addRegisterClass(MVT::i32, &OR1K::GPRRegClass);
  if (!TM.Options.UseSoftFloat)
    addRegisterClass(MVT::f32, &OR1K::GPRRegClass);

  // Compute derived properties from the register classes
  computeRegisterProperties();

  setStackPointerRegisterToSaveRestore(OR1K::R1);

  setOperationAction(ISD::BR_CC,             MVT::i32, Custom);
  setOperationAction(ISD::BR_CC,             MVT::f32, Custom);
  setOperationAction(ISD::BR_JT,             MVT::Other, Expand);
  setOperationAction(ISD::BRCOND,            MVT::Other, Expand);
  setOperationAction(ISD::SETCC,             MVT::i32, Expand);
  setOperationAction(ISD::SETCC,             MVT::f32, Expand);
  setOperationAction(ISD::SELECT,            MVT::i32, Expand);
  setOperationAction(ISD::SELECT,            MVT::f32, Expand);
  setOperationAction(ISD::SELECT_CC,         MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC,         MVT::f32, Custom);

  setOperationAction(ISD::GlobalAddress,     MVT::i32, Custom);
  setOperationAction(ISD::BlockAddress,      MVT::i32, Custom);
  setOperationAction(ISD::JumpTable,         MVT::i32, Custom);

  if (!TM.Options.UseSoftFloat)
    setOperationAction(ISD::ConstantFP,       MVT::f32,   Legal);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKSAVE,          MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE,       MVT::Other, Expand);

  setOperationAction(ISD::VASTART,            MVT::Other, Custom);
  setOperationAction(ISD::VAARG,              MVT::Other, Expand);
  setOperationAction(ISD::VACOPY,             MVT::Other, Custom);
  setOperationAction(ISD::VAEND,              MVT::Other, Expand);

  if (!Subtarget.hasDiv()) {
    setOperationAction(ISD::SDIV,            MVT::i32, Expand);
    setOperationAction(ISD::UDIV,            MVT::i32, Expand);
  }
  setOperationAction(ISD::SDIVREM,           MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM,           MVT::i32, Expand);
  setOperationAction(ISD::SREM,              MVT::i32, Expand);
  setOperationAction(ISD::UREM,              MVT::i32, Expand);

  if (!Subtarget.hasMul()) {
    setOperationAction(ISD::MUL,             MVT::i32, Expand);
  }

  if (!Subtarget.hasMul64()) {
    setOperationAction(ISD::MULHU,           MVT::i32, Expand);
    setOperationAction(ISD::MULHS,           MVT::i32, Expand);
    setOperationAction(ISD::UMUL_LOHI,       MVT::i32, Expand);
    setOperationAction(ISD::SMUL_LOHI,       MVT::i32, Expand);
  }

  setOperationAction(ISD::SUBC, MVT::i32, Expand);
  setOperationAction(ISD::SUBE, MVT::i32, Expand);

  if (!Subtarget.hasRor()) {
    setOperationAction(ISD::ROTR,            MVT::i32, Expand);
  }

  setOperationAction(ISD::ROTL,              MVT::i32, Expand);
  setOperationAction(ISD::SHL_PARTS,         MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS,         MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS,         MVT::i32, Expand);

  setOperationAction(ISD::BSWAP,             MVT::i32, Expand);
  if (Subtarget.hasFBit()) {
    setOperationAction(ISD::CTTZ,            MVT::i32, Custom);
    setOperationAction(ISD::CTLZ,            MVT::i32, Custom);
    setOperationAction(ISD::CTTZ_ZERO_UNDEF, MVT::i32, Custom);
    setOperationAction(ISD::CTLZ_ZERO_UNDEF, MVT::i32, Custom);
  } else {
    setOperationAction(ISD::CTTZ,            MVT::i32, Expand);
    setOperationAction(ISD::CTLZ,            MVT::i32, Expand);
    setOperationAction(ISD::CTTZ_ZERO_UNDEF, MVT::i32, Expand);
    setOperationAction(ISD::CTLZ_ZERO_UNDEF, MVT::i32, Expand);
  }
  setOperationAction(ISD::CTPOP,             MVT::i32, Expand);

  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1,   Expand);
  if (!Subtarget.hasExt()) {
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Expand);
  }

  // Extended load operations for i1 types must be promoted
  setLoadExtAction(ISD::EXTLOAD,             MVT::i1,   Promote);
  setLoadExtAction(ISD::ZEXTLOAD,            MVT::i1,   Promote);
  setLoadExtAction(ISD::SEXTLOAD,            MVT::i1,   Promote);

  setOperationAction(ISD::FP_TO_UINT,        MVT::i32,  Expand);
  setOperationAction(ISD::UINT_TO_FP,        MVT::i32,  Expand);

  // Function alignments (log2)
  setMinFunctionAlignment(2);
  setPrefFunctionAlignment(2);

  MaxStoresPerMemcpy = 16;
  MaxStoresPerMemcpyOptSize = 8;
  MaxStoresPerMemset = 16;
}

const char *OR1KTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  default: return nullptr;
  case OR1KISD::Return: return "OR1KISD::Return";
  case OR1KISD::Call: return "OR1KISD::Call";
  case OR1KISD::Select: return "OR1KISD::Select";
  case OR1KISD::SetFlag: return "OR1KISD::SetFlag";
  case OR1KISD::BrCond: return "OR1KISD::BrCond";
  case OR1KISD::FF1: return "OR1KISD::FF1";
  case OR1KISD::FL1: return "OR1KISD::FL1";
  case OR1KISD::HiLo: return "OR1KISD::HiLo";
  }
}

SDValue OR1KTargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Unexpected operation lowering!");
  case ISD::GlobalAddress: return LowerGlobalAddress(Op, DAG);
  case ISD::BlockAddress: return LowerBlockAddress(Op, DAG);
  case ISD::JumpTable: return LowerJumpTable(Op, DAG);
  case ISD::BR_CC: return LowerBR_CC(Op, DAG);
  case ISD::SELECT_CC: return LowerSELECT_CC(Op, DAG);
  case ISD::VASTART: return LowerVASTART(Op, DAG);
  case ISD::VACOPY: return LowerVACOPY(Op, DAG);
  case ISD::CTTZ: return LowerCTTZ(Op, DAG);
  case ISD::CTTZ_ZERO_UNDEF: return LowerCTTZ_ZERO_UNDEF(Op, DAG);
  case ISD::CTLZ: // fallthrough
  case ISD::CTLZ_ZERO_UNDEF: return LowerCTLZ(Op, DAG);
  case ISD::RETURNADDR: return LowerRETURNADDR(Op, DAG);
  case ISD::FRAMEADDR: return LowerFRAMEADDR(Op, DAG);
  }
}

//===----------------------------------------------------------------------===//
//                      Calling Convention Implementation
//===----------------------------------------------------------------------===//

namespace {

class OR1KCCState : public CCState {
public:
  OR1KCCState(CallingConv::ID CC, bool IsVarArg, MachineFunction &MF,
              const TargetMachine &TM, SmallVectorImpl<CCValAssign> &Locs,
              LLVMContext &C)
   : CCState(CC, IsVarArg, MF, TM, Locs, C), IsEndOfFixedArgs(false) {}

  void AnalyzeCallOperands(const SmallVectorImpl<ISD::OutputArg> &Outs,
                           CCAssignFn Fn);

  bool isEndOfFixedArgs() const { return IsEndOfFixedArgs; }

private:
  bool IsEndOfFixedArgs;
};

}

void OR1KCCState::
AnalyzeCallOperands(const SmallVectorImpl<ISD::OutputArg> &Outs,
                    CCAssignFn Fn) {
  unsigned NumOps = Outs.size();
  for (unsigned i = 0; i != NumOps; ++i) {
    MVT ArgVT = Outs[i].VT;
    ISD::ArgFlagsTy ArgFlags = Outs[i].Flags;
    IsEndOfFixedArgs = isVarArg() && !Outs[i].IsFixed;
    if (Fn(i, ArgVT, ArgVT, CCValAssign::Full, ArgFlags, *this)) {
#ifndef NDEBUG
      dbgs() << "Call operand #" << i << " has unhandled type "
             << EVT(ArgVT).getEVTString() << '\n';
#endif
      llvm_unreachable(0);
    }
  }
}

static bool CC_OR1K32_PairedArgs(unsigned ValNo, MVT ValVT, MVT LocVT,
                                 CCValAssign::LocInfo LocInfo,
                                 ISD::ArgFlagsTy ArgFlags, CCState &State) {
  assert(ArgFlags.isSplit() && "Unexpected non Split argument!");

  static const MCPhysReg Regs[] = {
    OR1K::R3, OR1K::R4, OR1K::R5,
    OR1K::R6, OR1K::R7, OR1K::R8
  };
  const unsigned NumRegs = array_lengthof(Regs);

  unsigned FirstUnalloc = State.getFirstUnallocated(Regs, NumRegs);

  // Check if there are at least two registers available.
  if (NumRegs - FirstUnalloc >= 2) {
    unsigned Reg = State.AllocateReg(Regs[FirstUnalloc]);
    assert(Reg && "Register already allocated?!");
    State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));
    return true;
  }

  // Not enough registers allocate the value on the stack shadowing all
  // registers.
  unsigned Offset = State.AllocateStack(4, 4, Regs, NumRegs);
  State.addLoc(CCValAssign::getMem(ValNo, ValVT, Offset, LocVT, LocInfo));

  return true;
}

#include "OR1KGenCallingConv.inc"

static SDValue HandleVarArgs_NewABI(SDValue Chain, OR1KCCState &CCInfo,
                                   SDLoc dl, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  FuncInfo->setVariadic(true);

  OR1KMachineFunctionInfo::VarArgsInfo &VAInfo = FuncInfo->getVarArgsInfo();

  int StackOffset = CCInfo.getNextStackOffset();
  VAInfo.StackAreaFI = MFI->CreateFixedObject(1, StackOffset, true);

  static const MCPhysReg Regs[] = {
    OR1K::R3, OR1K::R4, OR1K::R5,
    OR1K::R6, OR1K::R7, OR1K::R8
  };
  const unsigned NumRegs = array_lengthof(Regs);

  unsigned FirstUnalloc = CCInfo.getFirstUnallocated(Regs, NumRegs);

  // To minimaze the wasted stack space the reg save area is allocated
  // if non fixed arguments can be effectively passed by registers.

  if (FirstUnalloc == NumRegs) {
    VAInfo.RegSaveAreaFI = 0;
    VAInfo.AllocatedGPR = 24;
    return Chain;
  }

  unsigned RegSize = OR1K::GPRRegClass.getSize();
  unsigned RegSaveAreaSize = RegSize * (NumRegs - FirstUnalloc);
  int64_t RegSaveAreaOffset = -(int64_t)RegSaveAreaSize;

  FuncInfo->setRegSaveAreaSize(RegSaveAreaSize);
  VAInfo.AllocatedGPR = RegSize * FirstUnalloc;

  int FI = MFI->CreateFixedObject(RegSaveAreaSize, RegSaveAreaOffset, true);
  SDValue Base = DAG.getFrameIndex(FI, MVT::i32);
  VAInfo.RegSaveAreaFI = FI;

  int Offset = 0;
  for (; FirstUnalloc != NumRegs; ++FirstUnalloc, Offset += RegSize) {
    unsigned Reg = Regs[FirstUnalloc];
    unsigned VReg = MF.addLiveIn(Reg, &OR1K::GPRRegClass);

    SDValue Val = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
    SDValue Ptr = DAG.getNode(ISD::ADD, dl, MVT::i32, Base,
                              DAG.getConstant(Offset, MVT::i32));
    Chain = DAG.getStore(Chain, dl, Val, Ptr, MachinePointerInfo(),
                         false, false, 0);
  }

  return Chain;
}

SDValue OR1KTargetLowering::
LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                     bool IsVarArg, const SmallVectorImpl<ISD::InputArg> &Ins,
                     SDLoc dl, SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  OR1KCCState CCInfo(CallConv, IsVarArg, MF, getTargetMachine(),
                     ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_OR1K32);

  SDValue ArgsChain = DAG.getUNDEF(MVT::Other);

  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    EVT ValVT = VA.getValVT();
    ISD::ArgFlagsTy Flags = Ins[i].Flags;

    SDValue ArgValue;

    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      unsigned Reg = VA.getLocReg();

      assert((RegVT == MVT::i32 || RegVT == MVT::f32) &&
             "Unexpected register type!");

      unsigned VReg = MF.addLiveIn(Reg, &OR1K::GPRRegClass);
      ArgValue = DAG.getCopyFromReg(ArgsChain, dl, VReg, RegVT);

      if (Flags.isSRet())
        FuncInfo->setSRetReturnReg(VReg);

      switch (VA.getLocInfo()) {
      default:
        llvm_unreachable("Unsupported LocInfo!");
      case CCValAssign::Full:
        break;
      case CCValAssign::SExt:
        ArgValue = DAG.getNode(ISD::AssertSext, dl, RegVT, ArgValue,
                               DAG.getValueType(ValVT));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, ValVT, ArgValue);
        break;
      case CCValAssign::ZExt:
        ArgValue = DAG.getNode(ISD::AssertZext, dl, RegVT, ArgValue,
                               DAG.getValueType(ValVT));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, ValVT, ArgValue);
        break;
      }
    } else {
      assert(VA.isMemLoc());

      unsigned ObjSize = VA.getLocVT().getSizeInBits() / 8;
      unsigned ObjOffset = VA.getLocMemOffset();

      int FI = MFI->CreateFixedObject(ObjSize, ObjOffset, true);

      ArgValue = DAG.getLoad(VA.getLocVT(), dl, ArgsChain,
                             DAG.getFrameIndex(FI, MVT::i32),
                             MachinePointerInfo::getFixedStack(FI),
                             false, false, false, 0);
    }

    InVals.push_back(ArgValue);
  }

  if (IsVarArg && Subtarget.isNewABI())
    Chain = HandleVarArgs_NewABI(Chain, CCInfo, dl, DAG);

  if (IsVarArg && Subtarget.isDefaultABI()) {
    // The DefaultABI defines the passage of variadic parameter on the stack.
    FuncInfo->setVariadic(true);

    OR1KMachineFunctionInfo::VarArgsInfo &VAInfo = FuncInfo->getVarArgsInfo();
    int64_t VarArgsOffset = CCInfo.getNextStackOffset();
    VAInfo.VarArgsAreaFI = MFI->CreateFixedObject(1, VarArgsOffset, true);
  }

  // Relink arguments chain to the real chain.
  DAG.ReplaceAllUsesOfValueWith(ArgsChain, Chain);

  return Chain;
}

SDValue OR1KTargetLowering::
LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
            const SmallVectorImpl<ISD::OutputArg> &Outs,
            const SmallVectorImpl<SDValue> &OutVals,
            SDLoc dl, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();

  SmallVector<CCValAssign, 4> RVLocs;
  OR1KCCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(),
                     getTargetMachine(), RVLocs, *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_OR1K32);

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(),
                             OutVals[i], Flag);
    Flag = Chain.getValue(1);

    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  // Function that returns a struct by value must set in r11 the pointer
  // to the storage reserved by the callee for the value itself.
  if (FuncInfo->hasSRetReturnReg()) {
    unsigned VReg = FuncInfo->getSRetReturnReg();
    SDValue Val = DAG.getCopyFromReg(Chain, dl, VReg, getPointerTy());
    Chain = DAG.getCopyToReg(Chain, dl, OR1K::R11, Val, Flag);
    Flag = Chain.getValue(1);

    RetOps.push_back(DAG.getRegister(OR1K::R11, MVT::i32));
  }

  RetOps[0] = Chain;

  if (Flag.getNode())
    RetOps.push_back(Flag);

  return DAG.getNode(OR1KISD::Return, dl, MVT::Other, makeArrayRef(RetOps));
}

SDValue OR1KTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                      SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &dl = CLI.DL;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  auto TRI = static_cast<const OR1KRegisterInfo*>(TM.getRegisterInfo());

  CLI.IsTailCall = false;

  SmallVector<CCValAssign, 16> ArgLocs;
  OR1KCCState CCInfo(CLI.CallConv, CLI.IsVarArg, MF, getTargetMachine(),
                     ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(CLI.Outs, CC_OR1K32);

  unsigned NextStackOffset = CCInfo.getNextStackOffset();
  SDValue SPAdjAmount = DAG.getIntPtrConstant(NextStackOffset, true);

  Chain = DAG.getCALLSEQ_START(Chain, SPAdjAmount, dl);

  SDValue StackPtr = DAG.getCopyFromReg(Chain, dl, OR1K::R1, getPointerTy());

  SmallVector<std::pair<unsigned, SDValue>, 6> RegsToPass;
  SmallVector<SDValue, 12> MemOpChains;

  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    SDValue ArgValue = CLI.OutVals[i];
    CCValAssign &VA = ArgLocs[i];
    ISD::ArgFlagsTy Flags = CLI.Outs[i].Flags;
    MVT LocVT = VA.getLocVT();

    switch (VA.getLocInfo()) {
    default:
      llvm_unreachable("Unsupported LocInfo!");
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      ArgValue = DAG.getNode(ISD::SIGN_EXTEND, dl, LocVT, ArgValue);
      break;
    case CCValAssign::ZExt:
      ArgValue = DAG.getNode(ISD::ZERO_EXTEND, dl, LocVT, ArgValue);
      break;
    case CCValAssign::AExt:
      ArgValue = DAG.getNode(ISD::ANY_EXTEND, dl, LocVT, ArgValue);
      break;
    }

    if (Flags.isByVal()) {
      // On 'byval' arguments the caller must create a copy of the original
      // argument and pass a reference to the copy.
      unsigned Size = Flags.getByValSize();
      unsigned Align = Flags.getByValAlign();

      int FI = MFI->CreateStackObject(Size, Align, false);
      SDValue Addr = DAG.getFrameIndex(FI, getPointerTy());
      MemOpChains.push_back(DAG.getMemcpy(Chain, dl, Addr, ArgValue,
                                          DAG.getConstant(Size, MVT::i32),
                                          Align, false, false,
                                          MachinePointerInfo(),
                                          MachinePointerInfo()));
      ArgValue = Addr;
    }

    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), ArgValue));
    } else {
      assert(VA.isMemLoc());
      unsigned LocMemOffset = VA.getLocMemOffset();
      SDValue PtrOff = DAG.getIntPtrConstant(LocMemOffset);
      PtrOff = DAG.getNode(ISD::ADD, dl, getPointerTy(), StackPtr, PtrOff);
      SDValue MemOp = DAG.getStore(Chain, dl, ArgValue, PtrOff,
                                   MachinePointerInfo::getStack(LocMemOffset),
                                   false, false, 0);
      MemOpChains.push_back(MemOp);
    }
  }

  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
                        makeArrayRef(MemOpChains));

  // Build a sequence of copy-to-reg nodes chained together with token chain and
  // flag operands which copy the outgoing args into registers. The InFlag is
  // necessary since all emitted instructions must be stuck together.
  SDValue InFlag;
  for (const auto &Pair : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, dl, Pair.first, Pair.second, InFlag);
    InFlag = Chain.getValue(1);
  }

  bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;

  if (IsPIC) {
    SDValue GP = DAG.getGLOBAL_OFFSET_TABLE(MVT::i32);
    Chain = DAG.getCopyToReg(Chain, dl, TRI->getGlobalBaseRegister(),
                             GP, InFlag);
    InFlag = Chain.getValue(1);
  }

  // If the callee is a GlobalAddress node (quite common, every direct call is)
  // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
  // Likewise ExternalSymbol -> TargetExternalSymbol.
  uint8_t OpFlag = IsPIC ? OR1KII::MO_PLT26 : OR1KII::MO_NO_FLAG;
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, getPointerTy(),
                                        0, OpFlag);
  else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), getPointerTy(),
                                         OpFlag);

  SmallVector<SDValue, 16> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  for (const auto &Pair : RegsToPass)
    Ops.push_back(DAG.getRegister(Pair.first, Pair.second.getValueType()));

  if (IsPIC)
    Ops.push_back(DAG.getRegister(TRI->getGlobalBaseRegister(), MVT::i32));

  if (InFlag.getNode())
    Ops.push_back(InFlag);

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(OR1KISD::Call, dl, NodeTys, makeArrayRef(Ops));
  InFlag = Chain.getValue(1);

  Chain = DAG.getCALLSEQ_END(Chain, SPAdjAmount,
                             DAG.getIntPtrConstant(0, true), InFlag, dl);
  InFlag = Chain.getValue(1);

  return LowerCallResult(Chain, InFlag, CLI.CallConv, CLI.IsVarArg, CLI.Ins,
                         dl, DAG, InVals);
}


SDValue OR1KTargetLowering::
LowerCallResult(SDValue Chain, SDValue InFlag, CallingConv::ID CallConv,
                bool IsVarArg, const SmallVectorImpl<ISD::InputArg> &Ins,
                SDLoc dl, SelectionDAG &DAG,
                SmallVectorImpl<SDValue> &InVals) const {
  SmallVector<CCValAssign, 16> RVLocs;
  OR1KCCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(),
                     getTargetMachine(), RVLocs, *DAG.getContext());

  CCInfo.AnalyzeCallResult(Ins, RetCC_OR1K32);

  for (const CCValAssign &VA : RVLocs) {
    SDValue Value = DAG.getCopyFromReg(Chain, dl, VA.getLocReg(),
                                       VA.getValVT(), InFlag);
    Chain = Value.getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Value);
  }

  return Chain;
}

static SDValue getSimpleFloat32SetFlag(SDLoc dl, SDValue LHS, SDValue RHS,
                                       ISD::CondCode CC, bool &Negate,
                                       SelectionDAG &DAG) {
  switch (CC) {
  default:
    return SDValue();
  case ISD::SETOGT:
  case ISD::SETOLT:
  case ISD::SETOGE:
  case ISD::SETOLE:
  case ISD::SETOEQ:
  case ISD::SETUNE:
    Negate = false;
    break;
  case ISD::SETUGT:
    CC = ISD::SETOLE;
    Negate = true;
    break;
  case ISD::SETULT:
    CC = ISD::SETOGE;
    Negate = true;
    break;
  case ISD::SETUGE:
    CC = ISD::SETOLT;
    Negate = true;
    break;
  case ISD::SETULE:
    CC = ISD::SETOGT;
    Negate = true;
    break;
  }

  return DAG.getNode(OR1KISD::SetFlag, dl, MVT::Glue, LHS, RHS,
                     DAG.getCondCode(CC));
}

SDValue OR1KTargetLowering::getSetFlag(SDLoc dl, SDValue LHS, SDValue RHS,
                                       ISD::CondCode CC, bool &Negate,
                                       SelectionDAG &DAG) const {
  EVT VT = LHS.getValueType();

  Negate = false;

  if (VT == MVT::i32)
    return DAG.getNode(OR1KISD::SetFlag, dl, MVT::Glue, LHS, RHS,
                               DAG.getCondCode(CC));

  assert(VT == MVT::f32);

  SDValue Flag = getSimpleFloat32SetFlag(dl, LHS, RHS, CC, Negate, DAG);
  if (Flag.getNode())
    return Flag;

  if (CC == ISD::SETONE || CC == ISD::SETUEQ) {
    // To implement the SETONE comparison we use the following identity:
    //   SETONE(lhs, rhs) = SETOLT(lhs, rhs) or SETOGT(lhs, rhs)
    // Considering the limitation of the OR1K instruction set we have to use
    // a 'selectcc' in order to materialize the flags in order to combine them
    // through an 'or' operation.
    SDValue Zero = DAG.getConstant(0, MVT::i32);
    SDValue One = DAG.getConstant(1, MVT::i32);
    SDValue IsLT = DAG.getSelectCC(dl, LHS, RHS, One, Zero, ISD::SETOLT);
    SDValue IsGT = DAG.getSelectCC(dl, LHS, RHS, One, Zero, ISD::SETOGT);
    SDValue Cond = DAG.getNode(ISD::OR, dl, MVT::i32, IsLT, IsGT);

    // To implement the SETUEQ, we can just compute the SETONE comparison and
    // negate the flag.
    Negate = CC == ISD::SETUEQ;

    return DAG.getNode(OR1KISD::SetFlag, dl, MVT::Glue, Cond, Zero,
                       DAG.getCondCode(ISD::SETNE));
  }

  if (CC == ISD::SETO || CC == ISD::SETUO) {
    // To implement the SETO comparison we use the following identity:
    //   SETO(lhs, rhs) = SETOEQ(lhs, lhs) and SETOEQ(rhs, rhs)
    // Considering the limitation of the OR1K instruction set we have to use
    // a 'selectcc' in order to materialize the flags in order to combine them
    // through an 'or' operation.
    SDValue Zero = DAG.getConstant(0, MVT::i32);
    SDValue One = DAG.getConstant(1, MVT::i32);
    SDValue OrdLHS = DAG.getSelectCC(dl, LHS, LHS, One, Zero, ISD::SETOEQ);
    SDValue OrdRHS = DAG.getSelectCC(dl, RHS, RHS, One, Zero, ISD::SETOEQ);
    SDValue Cond = DAG.getNode(ISD::AND, dl, MVT::i32, OrdLHS, OrdRHS);

    // To implement the SET, we can just compute the SETONE comparison and
    // negate the flag.
    Negate = CC == ISD::SETUO;

    return DAG.getNode(OR1KISD::SetFlag, dl, MVT::Glue, Cond, Zero,
                       DAG.getCondCode(ISD::SETNE));
  }

  llvm_unreachable("Unknown SetFlag case!");
}

SDValue OR1KTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc dl(Op);
  SDValue Chain = Op.getOperand(0);
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();

  bool Negate = false;
  SDValue Glue = getSetFlag(dl, LHS, RHS, CC, Negate, DAG);
  SDValue Neg = DAG.getConstant(Negate ? 1 : 0, MVT::i32);

  return DAG.getNode(OR1KISD::BrCond, dl, MVT::Other, Chain, Dest, Neg, Glue);
}

SDValue OR1KTargetLowering::LowerSELECT_CC(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDLoc dl(Op);
  SDValue LHS  = Op.getOperand(0);
  SDValue RHS  = Op.getOperand(1);
  SDValue ValT = Op.getOperand(2);
  SDValue ValF = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();

  EVT VT = Op.getValueType();

  bool Swap = false;
  SDValue Glue = getSetFlag(dl, LHS, RHS, CC, Swap, DAG);

  if (Swap)
    std::swap(ValT, ValF);

  return DAG.getNode(OR1KISD::Select, dl, VT, ValT, ValF, Glue);
}

static SDValue LowerVASTART_NewABI(SDValue Op, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();
  OR1KMachineFunctionInfo::VarArgsInfo &VAInfo = FuncInfo->getVarArgsInfo();

  SDLoc dl(Op);
  SDValue Chain = Op.getOperand(0);
  SDValue VaListBasePtr = Op.getOperand(1);
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();

  SDValue Value, Ptr, Offset;
  MVT PtrVT = MVT::i32;

  // Initialize GPR size.
  Value = DAG.getConstant(VAInfo.AllocatedGPR, MVT::i32);
  Ptr = VaListBasePtr;
  Chain = DAG.getStore(Chain, dl, Value, Ptr, MachinePointerInfo(SV, 0),
                       false, false, 0);

  // Initialize stack area pointer.
  Value = DAG.getFrameIndex(VAInfo.StackAreaFI, PtrVT);
  Ptr = DAG.getNode(ISD::ADD, dl, PtrVT, VaListBasePtr,
                    DAG.getConstant(4, PtrVT));
  Chain = DAG.getStore(Chain, dl, Value, Ptr, MachinePointerInfo(SV, 4),
                       false, false, 0);

  // Initialize regsave area pointer.
  Value = VAInfo.AllocatedGPR < 24
            ? DAG.getFrameIndex(VAInfo.RegSaveAreaFI, PtrVT)
            : DAG.getConstant(0, PtrVT);
  Ptr = DAG.getNode(ISD::ADD, dl, PtrVT, VaListBasePtr,
                    DAG.getConstant(8, PtrVT));
  Chain = DAG.getStore(Chain, dl, Value, Ptr, MachinePointerInfo(SV, 8),
                       false, false, 0);

  return Chain;
}

static SDValue LowerVASTART_DefaultABI(SDValue Op, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  auto FuncInfo = MF.getInfo<OR1KMachineFunctionInfo>();
  OR1KMachineFunctionInfo::VarArgsInfo &VAInfo = FuncInfo->getVarArgsInfo();

  SDLoc dl(Op);
  SDValue Chain = Op.getOperand(0);
  SDValue VaListBasePtr = Op.getOperand(1);

  MVT PtrVT = MVT::i32;

  // We need to store the address of the VarArgsArea slot into the memory
  // location argument.
  SDValue Value = DAG.getFrameIndex(VAInfo.VarArgsAreaFI, PtrVT);
  const llvm::Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Chain, dl, Value, VaListBasePtr, MachinePointerInfo(SV),
                      false, false, 0);
}

SDValue OR1KTargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  if (Subtarget.isNewABI())
    return LowerVASTART_NewABI(Op, DAG);

  return LowerVASTART_DefaultABI(Op, DAG);
}

static SDValue LowerVACOPY_NewABI(SDValue Op, SelectionDAG &DAG) {
  SDValue Chain = Op.getOperand(0);
  SDValue DstPtr = Op.getOperand(1);
  SDValue SrcPtr = Op.getOperand(2);
  const Value *DstSV = cast<SrcValueSDNode>(Op.getOperand(3))->getValue();
  const Value *SrcSV = cast<SrcValueSDNode>(Op.getOperand(4))->getValue();
  SDLoc dl(Op);

  return DAG.getMemcpy(Chain, dl, DstPtr, SrcPtr,
                       DAG.getConstant(12, MVT::i32), 4, false, true,
                       MachinePointerInfo(DstSV), MachinePointerInfo(SrcSV));
}

SDValue OR1KTargetLowering::LowerVACOPY(SDValue Op, SelectionDAG &DAG) const {
  if (Subtarget.isNewABI())
    return LowerVACOPY_NewABI(Op, DAG);

  // Fallback on default expansion.
  return SDValue();
}

SDValue OR1KTargetLowering::LowerCTTZ(SDValue Op, SelectionDAG &DAG) const {
  SDLoc dl(Op);
  EVT VT = Op.getValueType();
  SDValue Value = Op.getOperand(0);
  SDValue ZeroUndef = DAG.getNode(ISD::CTTZ_ZERO_UNDEF, dl, VT, Value);
  SDValue NumBits = DAG.getConstant(VT.getSizeInBits(), MVT::i32);
  SDValue CC = DAG.getCondCode(ISD::SETEQ);

  // CTTZ = Value == 0 ? NumBits : CTTZ_undef
  return DAG.getNode(ISD::SELECT_CC, dl, VT, Value,
                     DAG.getConstant(0, MVT::i32), NumBits, ZeroUndef, CC);
}

SDValue OR1KTargetLowering::LowerCTTZ_ZERO_UNDEF(SDValue Op,
                                                 SelectionDAG &DAG) const {
  SDLoc dl(Op);
  EVT VT = Op.getValueType();

  // We use the relation CTTZ_undef = FF1 - 1.
  SDValue FF1 = DAG.getNode(OR1KISD::FF1, dl, VT, Op.getOperand(0));
  return DAG.getNode(ISD::SUB, dl, VT, FF1, DAG.getConstant(1, MVT::i32));
}

SDValue OR1KTargetLowering::LowerCTLZ(SDValue Op, SelectionDAG &DAG) const {
  SDLoc dl(Op);
  EVT VT = Op.getValueType();

  // We use the relation CTLZ = NumBits - FL1.
  SDValue FL1 = DAG.getNode(OR1KISD::FL1, dl, VT, Op.getOperand(0));
  SDValue NumBits = DAG.getConstant(VT.getSizeInBits(), MVT::i32);
  return DAG.getNode(ISD::SUB, dl, VT, NumBits, FL1);
}

SDValue OR1KTargetLowering::LowerRETURNADDR(SDValue Op,
                                            SelectionDAG &DAG) const {
  const TargetRegisterInfo *TRI = TM.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MFI->setReturnAddressIsTaken(true);

  const int64_t ReturnAddrFPOffset = -4;

  EVT VT = Op.getValueType();
  SDLoc dl(Op);
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  if (Depth) {
    SDValue FrameAddr = LowerFRAMEADDR(Op, DAG);
    SDValue Offset = DAG.getConstant(ReturnAddrFPOffset, MVT::i32);
    return DAG.getLoad(VT, dl, DAG.getEntryNode(),
                       DAG.getNode(ISD::ADD, dl, VT, FrameAddr, Offset),
                       MachinePointerInfo(), false, false, false, 0);
  }

  // Return the link register, which contains the return address.
  // Mark it an implicit live-in.
  unsigned Reg = MF.addLiveIn(TRI->getRARegister(), getRegClassFor(MVT::i32));
  return DAG.getCopyFromReg(DAG.getEntryNode(), dl, Reg, VT);
}

SDValue OR1KTargetLowering::LowerFRAMEADDR(SDValue Op,
                                           SelectionDAG &DAG) const {
  MachineFrameInfo *MFI = DAG.getMachineFunction().getFrameInfo();
  MFI->setFrameAddressIsTaken(true);

  const int64_t FrameAddrFPOffset = -8;

  EVT VT = Op.getValueType();
  SDLoc dl(Op);
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  SDValue FrameAddr = DAG.getCopyFromReg(DAG.getEntryNode(), dl, OR1K::R2, VT);
  SDValue Offset = DAG.getConstant(FrameAddrFPOffset, MVT::i32);
  while (Depth--)
    FrameAddr = DAG.getLoad(VT, dl, DAG.getEntryNode(),
                            DAG.getNode(ISD::ADD, dl, VT, FrameAddr, Offset),
                            MachinePointerInfo(), false, false, false, 0);
  return FrameAddr;
}

SDValue OR1KTargetLowering::LowerGlobalAddress(SDValue Op,
                                               SelectionDAG &DAG) const {
  SDLoc dl(Op);
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();
  bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;
  EVT PtrVT = getPointerTy();

  // Load GlobalAddress from GOT
  if (IsPIC && !GV->hasLocalLinkage() && !GV->hasHiddenVisibility()) {
    SDValue GA = DAG.getTargetGlobalAddress(GV, dl, PtrVT, Offset,
                                            OR1KII::MO_GOT16);
    return DAG.getLoad(PtrVT, dl, DAG.getEntryNode(), GA,
                       MachinePointerInfo(), false, false, false, 0);
  }

  uint8_t OpFlagHi = IsPIC ? OR1KII::MO_GOTOFF_HI16 : OR1KII::MO_ABS_HI16;
  uint8_t OpFlagLo = IsPIC ? OR1KII::MO_GOTOFF_LO16 : OR1KII::MO_ABS_LO16;

  // Create the TargetGlobalAddress node, folding in the constant offset.
  SDValue Hi = DAG.getTargetGlobalAddress(GV, dl, PtrVT, Offset, OpFlagHi);
  SDValue Lo = DAG.getTargetGlobalAddress(GV, dl, PtrVT, Offset, OpFlagLo);
  SDValue Result = DAG.getNode(OR1KISD::HiLo, dl, MVT::i32, Hi, Lo);

  if (IsPIC)
    Result = DAG.getNode(ISD::ADD, dl, MVT::i32, Result,
                         DAG.getGLOBAL_OFFSET_TABLE(MVT::i32));
  return Result;
}

SDValue OR1KTargetLowering::LowerBlockAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
  SDLoc dl(Op);
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();
  bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;

  uint8_t OpFlagHi = IsPIC ? OR1KII::MO_GOTOFF_HI16 : OR1KII::MO_ABS_HI16;
  uint8_t OpFlagLo = IsPIC ? OR1KII::MO_GOTOFF_LO16 : OR1KII::MO_ABS_LO16;

  SDValue Hi = DAG.getBlockAddress(BA, MVT::i32, true, OpFlagHi);
  SDValue Lo = DAG.getBlockAddress(BA, MVT::i32, true, OpFlagLo);
  SDValue Result = DAG.getNode(OR1KISD::HiLo, dl, MVT::i32, Hi, Lo);

  if (IsPIC)
    Result = DAG.getNode(ISD::ADD, dl, MVT::i32, Result,
                         DAG.getGLOBAL_OFFSET_TABLE(MVT::i32));
  return Result;
}

SDValue OR1KTargetLowering::LowerJumpTable(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDLoc dl(Op);
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Op);
  bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;
  EVT PtrVT = getPointerTy();

  uint8_t OpFlagHi = IsPIC ? OR1KII::MO_GOTOFF_HI16 : OR1KII::MO_ABS_HI16;
  uint8_t OpFlagLo = IsPIC ? OR1KII::MO_GOTOFF_LO16 : OR1KII::MO_ABS_LO16;

  SDValue Hi = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, OpFlagHi);
  SDValue Lo = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, OpFlagLo);
  SDValue Result = DAG.getNode(OR1KISD::HiLo, dl, MVT::i32, Hi, Lo);

  if (IsPIC)
    Result = DAG.getNode(ISD::ADD, dl, MVT::i32, Result,
                         DAG.getGLOBAL_OFFSET_TABLE(MVT::i32));
  return Result;
}

MachineBasicBlock *OR1KTargetLowering::
EmitInstrWithCustomInserter(MachineInstr *MI, MachineBasicBlock *BB) const {
  unsigned Opc = MI->getOpcode();

  const TargetInstrInfo &TII = *getTargetMachine().getInstrInfo();
  DebugLoc dl = MI->getDebugLoc();

  assert(Opc == OR1K::SELECT && "Unexpected instr type to insert");

  // To "insert" a SELECT instruction, we actually have to insert the diamond
  // control-flow pattern.  The incoming instruction knows the destination vreg
  // to set, the condition code register to branch on, the true/false values to
  // select between, and a branch opcode to use.
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator I = BB;
  ++I;

  //  thisMBB:
  //  ...
  //   TrueVal = ...
  //   l.sfXX r1, r2
  //   l.bf copy1MBB
  //   fallthrough --> copy0MBB
  MachineBasicBlock *thisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *copy1MBB = F->CreateMachineBasicBlock(LLVM_BB);

  F->insert(I, copy0MBB);
  F->insert(I, copy1MBB);
  // Update machine-CFG edges by transferring all successors of the current
  // block to the new block which will contain the Phi node for the select.
  copy1MBB->splice(copy1MBB->begin(), BB,
                   std::next(MachineBasicBlock::iterator(MI)), BB->end());
  copy1MBB->transferSuccessorsAndUpdatePHIs(BB);
  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(copy0MBB);
  BB->addSuccessor(copy1MBB);

  // Insert Branch if Flag
  BuildMI(BB, dl, TII.get(OR1K::BF)).addMBB(copy1MBB);

  //  copy0MBB:
  //   %FalseValue = ...
  //   # fallthrough to copy1MBB
  BB = copy0MBB;

  // Update machine-CFG edges
  BB->addSuccessor(copy1MBB);

  //  copy1MBB:
  //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
  //  ...
  BB = copy1MBB;
  BuildMI(*BB, BB->begin(), dl, TII.get(OR1K::PHI), MI->getOperand(0).getReg())
   .addReg(MI->getOperand(2).getReg()).addMBB(copy0MBB)
   .addReg(MI->getOperand(1).getReg()).addMBB(thisMBB);

  MI->eraseFromParent();   // The pseudo instruction is gone now.
  return BB;
}

bool OR1KTargetLowering::isLegalAddImmediate(int64_t Imm) const {
  // Immediates in add instruction are legal only if are at most 16 bits wide.
  return isInt<16>(Imm);
}

bool OR1KTargetLowering::isLegalICmpImmediate(int64_t Imm) const {
  // If the CPU supports SetFlagIf with Immediate instructions and the immediate
  // is at most 16 bits wide then it is legal.
  return Subtarget.hasSFII() && isInt<16>(Imm);
}

bool
OR1KTargetLowering::isLegalAddressingMode(const AddrMode& AM, Type* Ty) const {
  // No global is ever allowed as a base.
  if (AM.BaseGV)
    return false;

  switch (AM.Scale) {
  case 0: // "r+i" or just "i", depending on HasBaseReg.
    break;
  case 1:
    if (!AM.HasBaseReg) // allow "r+i".
      break;
    return false; // disallow "r+r" or "r+r+i".
  default:
    return false;
  }

  return true;
}

//===----------------------------------------------------------------------===//
//                       OR1K Inline Assembly Support
//===----------------------------------------------------------------------===//
std::pair<unsigned, const TargetRegisterClass*>
OR1KTargetLowering::getRegForInlineAsmConstraint(const std::string &Constraint,
                                                 MVT VT) const {
  if (Constraint.size() == 1) {
    // GCC Constraint Letters
    switch (Constraint[0]) {
    default: break;
    case 'r':   // GENERAL_REGS
      return std::make_pair(0U, &OR1K::GPRRegClass);
    }
  }

  return TargetLowering::getRegForInlineAsmConstraint(Constraint, VT);
}

/// Examine constraint type and operand type and determine a weight value.
/// This object must already have been set up with the operand type
/// and the current alternative constraint selected.
TargetLowering::ConstraintWeight OR1KTargetLowering::
getSingleConstraintMatchWeight(AsmOperandInfo &info,
                               const char *constraint) const {
  ConstraintWeight weight = CW_Invalid;
  Value *CallOperandVal = info.CallOperandVal;
    // If we don't have a value, we can't do a match,
    // but allow it at the lowest weight.
  if (CallOperandVal == NULL)
    return CW_Default;
  // Look at the constraint type.
  switch (*constraint) {
  default:
    weight = TargetLowering::getSingleConstraintMatchWeight(info, constraint);
    break;
  case 'I': // signed 16 bit immediate
  case 'J': // integer zero
  case 'K': // unsigned 16 bit immediate
  case 'L': // immediate in the range 0 to 31
  case 'M': // signed 32 bit immediate where lower 16 bits are 0
  case 'N': // signed 26 bit immediate
  case 'O': // integer zero
    if (isa<ConstantInt>(CallOperandVal))
      weight = CW_Constant;
    break;
  }
  return weight;
}

/// LowerAsmOperandForConstraint - Lower the specified operand into the Ops
/// vector.  If it is invalid, don't add anything to Ops.
void OR1KTargetLowering::LowerAsmOperandForConstraint(SDValue Op,
                                                      std::string &Constraint,
                                                      std::vector<SDValue>&Ops,
                                                      SelectionDAG &DAG) const {
  SDValue Result(0, 0);

  // Only support length 1 constraints for now.
  if (Constraint.length() > 1) return;

  char ConstraintLetter = Constraint[0];
  switch (ConstraintLetter) {
  default: break; // This will fall through to the generic implementation
  case 'I': // Signed 16 bit constant
    // If this fails, the parent routine will give an error
    if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
      if (isInt<16>(C->getSExtValue())) {
        Result = DAG.getTargetConstant(C->getSExtValue(), Op.getValueType());
        break;
      }
    }
    return;
  case 'J': // integer zero
  case 'O':
    if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
      if (C->getZExtValue() == 0) {
        Result = DAG.getTargetConstant(0, Op.getValueType());
        break;
      }
    }
    return;
  case 'K': // unsigned 16 bit immediate
    if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
      if (isUInt<16>(C->getZExtValue())) {
        Result = DAG.getTargetConstant(C->getSExtValue(), Op.getValueType());
        break;
      }
    }
    return;
  case 'L': // immediate in the range 0 to 31
    if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
      if (isUInt<5>(C->getZExtValue())) {
        Result = DAG.getTargetConstant(C->getZExtValue(), Op.getValueType());
        break;
      }
    }
    return;
  case 'M': // signed 32 bit immediate where lower 16 bits are 0
    if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
      uint64_t Val = C->getSExtValue();
      if (isShiftedUInt<16, 16>(Val)) {
        Result = DAG.getTargetConstant(Val, Op.getValueType());
        break;
      }
    }
    return;
  case 'N': // signed 26 bit immediate
    if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
      int64_t Val = C->getSExtValue();
      if (isInt<26>(Val)) {
        Result = DAG.getTargetConstant(Val, Op.getValueType());
        break;
      }
    }
    return;
  }

  if (Result.getNode()) {
    Ops.push_back(Result);
    return;
  }

  TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops, DAG);
}
