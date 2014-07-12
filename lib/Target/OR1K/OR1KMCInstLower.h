//===-- OR1KMCInstLower.h - Lower MachineInstr to MCInst --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef OR1K_MCINSTLOWER_H
#define OR1K_MCINSTLOWER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
class AsmPrinter;
class MCContext;
class MCInst;
class MCOperand;
class MCSymbol;
class MachineInstr;
class MachineModuleInfoMachO;
class MachineOperand;

/// \brief This class is used to lower an MachineInstr into an MCInst.
class LLVM_LIBRARY_VISIBILITY OR1KMCInstLower {
public:
  OR1KMCInstLower(MCContext &ctx, AsmPrinter &printer)
      : Ctx(ctx), Printer(printer) {}

  void lower(const MachineInstr *MI, MCInst &OutMI) const;

private:
  MCOperand lowerSymbolOperand(const MachineOperand &MO) const;
  MCSymbol *getSymbolForOperand(const MachineOperand &MO) const;

private:
  MCContext &Ctx;
  AsmPrinter &Printer;
};
}

#endif
