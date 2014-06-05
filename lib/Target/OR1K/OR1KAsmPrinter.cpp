//===-- OR1KAsmPrinter.cpp - OR1K LLVM assembly writer --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the OR1K assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "OR1K.h"
#include "OR1KInstrInfo.h"
#include "OR1KMCInstLower.h"
#include "OR1KTargetMachine.h"
#include "InstPrinter/OR1KInstPrinter.h"
#include "MCTargetDesc/OR1KMCExpr.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
using namespace llvm;

namespace {

class OR1KAsmPrinter : public AsmPrinter {
public:
  explicit OR1KAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
    : AsmPrinter(TM, Streamer) {}

  virtual const char *getPassName() const {
    return "OR1K Assembly Printer";
  }

  void printOperand(const MachineInstr *MI, int OpNum,
                    raw_ostream &O, const char* Modifier = 0);
  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       unsigned AsmVariant, const char *ExtraCode,
                       raw_ostream &O);
  void EmitInstruction(const MachineInstr *MI);
  virtual bool isBlockOnlyReachableByFallthrough(const MachineBasicBlock*
                                                 MBB) const;
private:
  void lowerGET_GLOBAL_BASE(const MachineInstr *MI);
};

}

void OR1KAsmPrinter::printOperand(const MachineInstr *MI, int OpNum,
                                  raw_ostream &O, const char *Modifier) {
  const MachineOperand &MO = MI->getOperand(OpNum);
  unsigned TF = MO.getTargetFlags();

  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << OR1KInstPrinter::getRegisterName(MO.getReg());
    break;

  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;

  case MachineOperand::MO_MachineBasicBlock:
    O << *MO.getMBB()->getSymbol();
    break;

  case MachineOperand::MO_GlobalAddress:
    if (TF == OR1KII::MO_PLT26)
      O << "plt(" << *getSymbol(MO.getGlobal()) << ")";
    else
      O << *getSymbol(MO.getGlobal());
    break;

  case MachineOperand::MO_BlockAddress:
     O << GetBlockAddressSymbol(MO.getBlockAddress())->getName();
     break;

  case MachineOperand::MO_ExternalSymbol:
    if (TF == OR1KII::MO_PLT26)
      O << "plt(" << *GetExternalSymbolSymbol(MO.getSymbolName()) << ")";
    else
      O << *GetExternalSymbolSymbol(MO.getSymbolName());
    break;

  case MachineOperand::MO_JumpTableIndex:
    O << *GetJTISymbol(MO.getIndex());
    break;

  case MachineOperand::MO_ConstantPoolIndex:
    O << *GetCPISymbol(MO.getIndex());
    return;

  default:
    llvm_unreachable("<unknown operand type>");
  }
}

/// PrintAsmOperand - Print out an operand for an inline asm expression.
///
bool OR1KAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                     unsigned AsmVariant,
                                     const char *ExtraCode, raw_ostream &O) {
  // Does this asm operand have a single letter operand modifier?
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1])
      return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    default:
      return true; // Unknown modifier.
    case 'H': // The highest-numbered register of a pair.
      if (OpNo == 0)
        return true;
      const MachineOperand &FlagsOP = MI->getOperand(OpNo - 1);
      if (!FlagsOP.isImm())
        return true;
      unsigned Flags = FlagsOP.getImm();
      unsigned NumVals = InlineAsm::getNumOperandRegisters(Flags);
      if (NumVals != 2)
        return true;
      unsigned RegOp = OpNo + 1;
      if (RegOp >= MI->getNumOperands())
        return true;
      const MachineOperand &MO = MI->getOperand(RegOp);
      if (!MO.isReg())
        return true;
      unsigned Reg = MO.getReg();
      O << OR1KInstPrinter::getRegisterName(Reg);
      return false;
    }
  }
  printOperand(MI, OpNo, O);
  return false;
}

void OR1KAsmPrinter::lowerGET_GLOBAL_BASE(const MachineInstr *MI) {
  unsigned GlobalBaseReg = MI->getOperand(0).getReg();

  switch (TM.getRelocationModel()) {
  default: llvm_unreachable("Unexpected relocation model for GET_GLOBAL_BASE");
  case Reloc::PIC_: {
    // Computation of the global base relative of a given code location.
    //
    //     jal 8
    //     movhi RX, gotoffhi(_GLOBAL_OFFSET_TABLE_ - 4)
    //     ori RX, RX, gotofflo(_GLOBAL_OFFSET_TABLE_ + 0)
    //     add RX, RX, R9
    //

    StringRef GOTName("_GLOBAL_OFFSET_TABLE_");
    const MCExpr *GOT =
      MCSymbolRefExpr::Create(OutContext.GetOrCreateSymbol(GOTName),
                              MCSymbolRefExpr::VK_None, OutContext);

    MCInst I1, I2, I3, I4;
    I1.setOpcode(OR1K::JAL);
    I1.addOperand(MCOperand::CreateImm(8));
    OutStreamer.EmitInstruction(I1, getSubtargetInfo());

    I2.setOpcode(OR1K::MOVHI);
    I2.addOperand(MCOperand::CreateReg(GlobalBaseReg));
    const MCExpr *HiOffset = MCConstantExpr::Create(4, OutContext);
    const MCExpr *Hi = MCBinaryExpr::CreateSub(GOT, HiOffset, OutContext);
    Hi = OR1KMCExpr::Create(OR1KMCExpr::VK_OR1K_GOTPC_HI16, Hi, OutContext);
    I2.addOperand(MCOperand::CreateExpr(Hi));
    OutStreamer.EmitInstruction(I2, getSubtargetInfo());

    I3.setOpcode(OR1K::ORI);
    I3.addOperand(MCOperand::CreateReg(GlobalBaseReg));
    I3.addOperand(MCOperand::CreateReg(GlobalBaseReg));
    const MCExpr *Lo = OR1KMCExpr::Create(OR1KMCExpr::VK_OR1K_GOTPC_LO16,
                                          GOT, OutContext);
    I3.addOperand(MCOperand::CreateExpr(Lo));
    OutStreamer.EmitInstruction(I3, getSubtargetInfo());

    I4.setOpcode(OR1K::ADD);
    I4.addOperand(MCOperand::CreateReg(GlobalBaseReg));
    I4.addOperand(MCOperand::CreateReg(GlobalBaseReg));
    I4.addOperand(MCOperand::CreateReg(OR1K::R9));
    OutStreamer.EmitInstruction(I4, getSubtargetInfo());
  }
  }

}

void OR1KAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  OR1KMCInstLower MCInstLowering(OutContext, *this);

  MachineBasicBlock::const_instr_iterator I = MI;
  MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();

  // Emit instruction and associated delay slots.
  do {
    switch (I->getOpcode()) {
    case OR1K::GET_GLOBAL_BASE:
      return lowerGET_GLOBAL_BASE(I);
    default: break;
    }

    MCInst TmpInst;
    MCInstLowering.lower(I, TmpInst);
    OutStreamer.EmitInstruction(TmpInst, getSubtargetInfo());
  } while ((++I != E) && I->isInsideBundle());
}

/// isBlockOnlyReachableByFallthough - Return true if the basic block has
/// exactly one predecessor and the control transfer mechanism between
/// the predecessor and this block is a fall-through.
// FIXME: could the overridden cases be handled in AnalyzeBranch?
bool OR1KAsmPrinter::isBlockOnlyReachableByFallthrough(const MachineBasicBlock*
                                                       MBB) const {
  // The predecessor has to be immediately before this block.
  const MachineBasicBlock *Pred = *MBB->pred_begin();

  // If the predecessor is a switch statement, assume a jump table
  // implementation, so it is not a fall through.
  if (const BasicBlock *bb = Pred->getBasicBlock())
    if (isa<SwitchInst>(bb->getTerminator()))
      return false;

  // Check default implementation
  if (!AsmPrinter::isBlockOnlyReachableByFallthrough(MBB))
    return false;

  // Otherwise, check the last instruction.
  // Check if the last terminator is an unconditional branch.
  MachineBasicBlock::const_iterator I = Pred->end();
  while (I != Pred->begin() && !(--I)->isTerminator()) ;

  return !I->isBarrier();
}

// Force static initialization.
extern "C" void LLVMInitializeOR1KAsmPrinter() {
  RegisterAsmPrinter<OR1KAsmPrinter> X(TheOR1KbeTarget);
  RegisterAsmPrinter<OR1KAsmPrinter> Y(TheOR1KleTarget);
}
