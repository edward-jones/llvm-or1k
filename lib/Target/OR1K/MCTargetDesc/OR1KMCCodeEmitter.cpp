//===-- OR1KMCCodeEmitter.cpp - Convert OR1K code to machine code ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the OR1KMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "mccodeemitter"
#include "MCTargetDesc/OR1KBaseInfo.h"
#include "MCTargetDesc/OR1KFixupKinds.h"
#include "MCTargetDesc/OR1KMCExpr.h"
#include "MCTargetDesc/OR1KMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/MCContext.h"
using namespace llvm;

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

namespace {
class OR1KMCCodeEmitter : public MCCodeEmitter {
  OR1KMCCodeEmitter(const OR1KMCCodeEmitter &); // DO NOT IMPLEMENT
  void operator=(const OR1KMCCodeEmitter &); // DO NOT IMPLEMENT
  const MCInstrInfo &MCII;
  const MCSubtargetInfo &STI;
  MCContext &Ctx;
  bool IsLittleEndian;

public:
  OR1KMCCodeEmitter(const MCInstrInfo &mcii, const MCSubtargetInfo &sti,
                    MCContext &ctx, bool IsLittleEndian)
    : MCII(mcii), STI(sti), Ctx(ctx), IsLittleEndian(IsLittleEndian) {
    }

  ~OR1KMCCodeEmitter() {}

  // getBinaryCodeForInstr - TableGen'erated function for getting the
  // binary encoding for an instruction.
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

   // getMachineOpValue - Return binary encoding of operand. If the machin
   // operand requires relocation, record the relocation and return zero.
  unsigned getMachineOpValue(const MCInst &MI,const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getMemoryOpValue(const MCInst &MI, unsigned Op,
                            SmallVectorImpl<MCFixup> &Fixups,
                            const MCSubtargetInfo &STI) const;

  // Emit one byte through output stream (from MCBlazeMCCodeEmitter)
  void EmitByte(unsigned char C, unsigned &CurByte, raw_ostream &OS) const {
    OS << (char)C;
    ++CurByte;
  }

  // Emit a series of bytes (little endian) (from MCBlazeMCCodeEmitter)
  void EmitLEConstant(uint64_t Val, unsigned Size, unsigned &CurByte,
                    raw_ostream &OS) const {
    assert(Size <= 8 && "size too big in emit constant");

    for (unsigned i = 0; i != Size; ++i) {
      EmitByte(Val & 255, CurByte, OS);
      Val >>= 8;
    }
  }

  // Emit a series of bytes (big endian)
  void EmitBEConstant(uint64_t Val, unsigned Size, unsigned &CurByte,
                      raw_ostream &OS) const {
    assert(Size <= 8 && "size too big in emit constant");

    for (int i = (Size-1)*8; i >= 0; i-=8)
      EmitByte((Val >> i) & 255, CurByte, OS);
  }

  void EncodeInstruction(const MCInst &Inst, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const;
};
} // end anonymous namepsace

MCCodeEmitter *llvm::createOR1KbeMCCodeEmitter(const MCInstrInfo &MCII,
                                               const MCRegisterInfo &MRI,
                                               const MCSubtargetInfo &STI,
                                               MCContext &Ctx) {
  return new OR1KMCCodeEmitter(MCII, STI, Ctx, false);
}

MCCodeEmitter *llvm::createOR1KleMCCodeEmitter(const MCInstrInfo &MCII,
                                               const MCRegisterInfo &MRI,
                                               const MCSubtargetInfo &STI,
                                               MCContext &Ctx) {
  return new OR1KMCCodeEmitter(MCII, STI, Ctx, true);
}

static OR1K::Fixups getFixupKind(OR1KMCExpr::VariantKind VK) {
  switch (VK) {
  case OR1KMCExpr::VK_OR1K_REL26: return OR1K::fixup_OR1K_REL26;
  case OR1KMCExpr::VK_OR1K_ABS_HI16: return OR1K::fixup_OR1K_HI16_INSN;
  case OR1KMCExpr::VK_OR1K_ABS_LO16: return OR1K::fixup_OR1K_LO16_INSN;
  case OR1KMCExpr::VK_OR1K_PLT26: return OR1K::fixup_OR1K_PLT26;
  case OR1KMCExpr::VK_OR1K_GOT16: return OR1K::fixup_OR1K_GOT16;
  case OR1KMCExpr::VK_OR1K_GOTPC_HI16: return OR1K::fixup_OR1K_GOTPC_HI16;
  case OR1KMCExpr::VK_OR1K_GOTPC_LO16: return OR1K::fixup_OR1K_GOTPC_LO16;
  case OR1KMCExpr::VK_OR1K_GOTOFF_HI16: return OR1K::fixup_OR1K_GOTOFF_HI16;
  case OR1KMCExpr::VK_OR1K_GOTOFF_LO16: return OR1K::fixup_OR1K_GOTOFF_LO16;
  default: break;
  }
  llvm_unreachable("Unknown fixup!");
}

/// getMachineOpValue - Return binary encoding of operand. If the machine
/// operand requires relocation, record the relocation and return zero.
unsigned OR1KMCCodeEmitter::
getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());
  
  // MO must be an expression
  assert(MO.isExpr());

  const OR1KMCExpr *Expr = cast<OR1KMCExpr>(MO.getExpr());

  OR1K::Fixups FixupKind = getFixupKind(Expr->getVariantKind());

  // Push fixup (all info is contained within)
  Fixups.push_back(MCFixup::Create(0, MO.getExpr(), MCFixupKind(FixupKind)));
  return 0;
}

void OR1KMCCodeEmitter::
EncodeInstruction(const MCInst &Inst, raw_ostream &OS,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const {
  // Keep track of the current byte being emitted
  unsigned CurByte = 0;

  // Get instruction encoding and emit it
  ++MCNumEmitted;       // Keep track of the number of emitted insns.
  unsigned Val = getBinaryCodeForInstr(Inst, Fixups, STI);
  IsLittleEndian ?
    EmitLEConstant(Val, 4, CurByte, OS) : EmitBEConstant(Val, 4, CurByte, OS);
}

// Encode OR1K Memory Operand
unsigned
OR1KMCCodeEmitter::getMemoryOpValue(const MCInst &MI, unsigned Op,
                                    SmallVectorImpl<MCFixup> &Fixups,
                                    const MCSubtargetInfo &STI) const {
  unsigned Encoding = 0;
  unsigned BaseReg = MI.getOperand(1).getReg();
  Encoding = Ctx.getRegisterInfo()->getEncodingValue(BaseReg) << 16;

  const MCOperand MO = MI.getOperand(2);
  if (MO.isImm())
    return Encoding |= MO.getImm() & 0xffff;

  const OR1KMCExpr *Expr = cast<OR1KMCExpr>(MO.getExpr());

  OR1K::Fixups FixupKind = getFixupKind(Expr->getVariantKind());

  // Push fixup (all info is contained within)
  Fixups.push_back(MCFixup::Create(0, MO.getExpr(), MCFixupKind(FixupKind)));

  return Encoding;
}

#include "OR1KGenMCCodeEmitter.inc"
