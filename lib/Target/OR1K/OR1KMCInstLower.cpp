//===-- OR1KMCInstLower.cpp - Convert MachineInstr to MCInst ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower OR1K MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "OR1KMCInstLower.h"
#include "MCTargetDesc/OR1KBaseInfo.h"
#include "MCTargetDesc/OR1KMCExpr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Constants.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "or1k-mcinstr-lower"

using namespace llvm;

MCSymbol *OR1KMCInstLower::getSymbolForOperand(const MachineOperand &MO) const {
  switch (MO.getType()) {
  case MachineOperand::MO_MachineBasicBlock:
    return MO.getMBB()->getSymbol();
  case MachineOperand::MO_GlobalAddress:
    return Printer.getSymbol(MO.getGlobal());
  case MachineOperand::MO_BlockAddress:
    return Printer.GetBlockAddressSymbol(MO.getBlockAddress());
  case MachineOperand::MO_ExternalSymbol:
    return Printer.GetExternalSymbolSymbol(MO.getSymbolName());
  case MachineOperand::MO_JumpTableIndex:
    return Printer.GetJTISymbol(MO.getIndex());
  default:
    break;
  }
  llvm_unreachable("invalid operand!");
}

static OR1KMCExpr::VariantKind getVariantKind(const MachineOperand &MO) {
  const MachineInstr *MI = MO.getParent();
  switch (MO.getTargetFlags()) {
  case OR1KII::MO_NO_FLAG:
    assert(MI->isCall() || MI->isBranch());
    return OR1KMCExpr::VK_OR1K_REL26;
  case OR1KII::MO_ABS_HI16:
    return OR1KMCExpr::VK_OR1K_ABS_HI16;
  case OR1KII::MO_ABS_LO16:
    return OR1KMCExpr::VK_OR1K_ABS_LO16;
  case OR1KII::MO_PLT26:
    return OR1KMCExpr::VK_OR1K_PLT26;
  case OR1KII::MO_GOT16:
    return OR1KMCExpr::VK_OR1K_GOT16;
  case OR1KII::MO_GOTPC_HI16:
    return OR1KMCExpr::VK_OR1K_GOTPC_HI16;
  case OR1KII::MO_GOTPC_LO16:
    return OR1KMCExpr::VK_OR1K_GOTPC_LO16;
  case OR1KII::MO_GOTOFF_HI16:
    return OR1KMCExpr::VK_OR1K_GOTOFF_HI16;
  case OR1KII::MO_GOTOFF_LO16:
    return OR1KMCExpr::VK_OR1K_GOTOFF_LO16;
  default:
    llvm_unreachable("invalid target flags!");
  }
}

MCOperand OR1KMCInstLower::lowerSymbolOperand(const MachineOperand &MO) const {
  OR1KMCExpr::VariantKind Kind = getVariantKind(MO);

  const MCExpr *Expr = MCSymbolRefExpr::Create(getSymbolForOperand(MO),
                                               MCSymbolRefExpr::VK_None, Ctx);

  if (!MO.isJTI() && !MO.isMBB()) {
    int64_t Offset = MO.getOffset();

    if (Offset)
      Expr = MCBinaryExpr::CreateAdd(Expr, MCConstantExpr::Create(Offset, Ctx),
                                     Ctx);
  }

  if (Kind != OR1KMCExpr::VK_None)
    Expr = OR1KMCExpr::Create(Kind, Expr, Ctx);

  return MCOperand::CreateExpr(Expr);
}

void OR1KMCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);

    MCOperand MCOp;
    switch (MO.getType()) {
    default:
      MI->dump();
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_Register:
      if (MO.isImplicit())
        continue;
      MCOp = MCOperand::CreateReg(MO.getReg());
      break;
    case MachineOperand::MO_FPImmediate: {
      APFloat Val = MO.getFPImm()->getValueAPF();
      // FP immediates are used only when setting GPRs, so they may be dealt
      // with like regular immediates from this point on.
      MCOp = MCOperand::CreateImm(*Val.bitcastToAPInt().getRawData());
      break;
    }
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::CreateImm(MO.getImm());
      break;
    case MachineOperand::MO_MachineBasicBlock:
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_BlockAddress:
    case MachineOperand::MO_ExternalSymbol:
    case MachineOperand::MO_JumpTableIndex:
      MCOp = lowerSymbolOperand(MO);
      break;
    }

    OutMI.addOperand(MCOp);
  }
}
