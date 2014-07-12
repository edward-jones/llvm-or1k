//===-- OR1KAsmBackend.cpp - OR1K Assembler Backend -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "OR1KFixupKinds.h"
#include "MCTargetDesc/OR1KMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "or1k-asm-backend"

using namespace llvm;

namespace {
class OR1KAsmBackend : public MCAsmBackend {
  Triple::OSType OSType;
  bool IsLittleEndian;

public:
  OR1KAsmBackend(const Target &T, Triple::OSType _OSType, bool IsLittleEndian)
      : MCAsmBackend(), OSType(_OSType), IsLittleEndian(IsLittleEndian) {}

  void applyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                  uint64_t Value, bool IsPCRel) const;

  MCObjectWriter *createObjectWriter(raw_ostream &OS) const;

  // No instruction requires relaxation
  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const {
    return false;
  }

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const;

  unsigned getNumFixupKinds() const { return OR1K::NumTargetFixupKinds; }

  bool mayNeedRelaxation(const MCInst &Inst) const { return false; }

  void relaxInstruction(const MCInst &Inst, MCInst &Res) const {}

  bool writeNopData(uint64_t Count, MCObjectWriter *OW) const;
};
} // end anonymous namespace

bool OR1KAsmBackend::writeNopData(uint64_t Count, MCObjectWriter *OW) const {
  if ((Count % 4) != 0)
    return false;

  for (uint64_t i = 0; i < Count; i += 4)
    OW->Write32(0x15000000);

  return true;
}

// Prepare value for the target space
static unsigned adjustFixupValue(unsigned Kind, uint64_t Value) {
  switch (Kind) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
    break;
  case OR1K::fixup_OR1K_REL26:
  case OR1K::fixup_OR1K_PLT26:
    // Currently this is used only for branches
    // Branch instructions require the value shifted down to to provide
    // a larger address range that can be branched to.
    Value >>= 2;
    break;
  // Values taken from BFD Relocation definitions
  case OR1K::fixup_OR1K_HI16_INSN:
  case OR1K::fixup_OR1K_GOTPC_HI16:
  case OR1K::fixup_OR1K_GOTOFF_HI16:
    Value >>= 16;
  case OR1K::fixup_OR1K_LO16_INSN:
  case OR1K::fixup_OR1K_GOT16:
  case OR1K::fixup_OR1K_GOTPC_LO16:
  case OR1K::fixup_OR1K_GOTOFF_LO16:
    Value &= 0xffff;
    break;
  }
  return Value;
}

void OR1KAsmBackend::applyFixup(const MCFixup &Fixup, char *Data,
                                unsigned DataSize, uint64_t Value,
                                bool IsPCRel) const {
  MCFixupKind Kind = Fixup.getKind();
  Value = adjustFixupValue(Kind, Value);

  if (!Value)
    return; // This value doesn't change the encoding

  // Where in the object and where the number of bytes that need
  // fixing up
  unsigned Offset = Fixup.getOffset();
  unsigned NumBits = getFixupKindInfo(Kind).TargetSize;
  unsigned NumBytes = (NumBits + 7) / 8;

  const unsigned InstrSizeInBytes = 4;

  // Keep fixup bits only.
  Value &= (uint64_t(1) << NumBits) - 1;

  // Write out the fixed up bytes back to the code/data bits.
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned ShAmtByte = IsLittleEndian ? i : (InstrSizeInBytes - i - 1);
    Data[Offset + i] |= (uint8_t)(Value >> (ShAmtByte * 8));
  }
}

MCObjectWriter *OR1KAsmBackend::createObjectWriter(raw_ostream &OS) const {
  return createOR1KELFObjectWriter(
      OS, MCELFObjectTargetWriter::getOSABI(OSType), IsLittleEndian);
}

const MCFixupKindInfo &
OR1KAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  const static MCFixupKindInfo BigEndianInfos[OR1K::NumTargetFixupKinds] = {
      // This table *must* be in same the order of fixup_* kinds in
      // OR1KFixupKinds.h.
      //
      // name                    offset  bits  flags
    { "fixup_OR1K_NONE",         0,      32,   0 },
    { "fixup_OR1K_32",           0,      32,   0 },
    { "fixup_OR1K_16",           0,      16,   0 },
    { "fixup_OR1K_8",            0,       8,   0 },
    { "fixup_OR1K_LO16_INSN",    0,      16,   0 },
    { "fixup_OR1K_HI16_INSN",    0,      16,   0 },
    { "fixup_OR1K_REL26",        0,      26,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_PCREL32",      0,      32,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_PCREL16",      0,      16,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_PCREL8",       0,       8,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOTPC_HI16",   0,      16,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOTPC_LO16",   0,      16,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOT16",        0,      16,   0 },
    { "fixup_OR1K_PLT26",        0,      26,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOTOFF_HI16",  0,      16,   0 },
    { "fixup_OR1K_GOTOFF_LO16",  0,      16,   0 },
    { "fixup_OR1K_COPY",         0,      32,   0 },
    { "fixup_OR1K_GLOB_DAT",     0,      32,   0 },
    { "fixup_OR1K_JMP_SLOT",     0,      32,   0 },
    { "fixup_OR1K_RELATIVE",     0,      32,   0 }
  };

  const static MCFixupKindInfo LittleEndianInfos[OR1K::NumTargetFixupKinds] = {
      // This table *must* be in same the order of fixup_* kinds in
      // OR1KFixupKinds.h.
      //
      // name                    offset  bits  flags
    { "fixup_OR1K_NONE",         0,      32,   0 },
    { "fixup_OR1K_32",           0,      32,   0 },
    { "fixup_OR1K_16",          16,      16,   0 },
    { "fixup_OR1K_8",           24,       8,   0 },
    { "fixup_OR1K_LO16_INSN",   16,      16,   0 },
    { "fixup_OR1K_HI16_INSN",   16,      16,   0 },
    { "fixup_OR1K_REL26",        6,      26,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_PCREL32",      0,      32,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_PCREL16",     16,      16,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_PCREL8",      24,       8,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOTPC_HI16",  16,      16,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOTPC_LO16",  16,      16,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOT16",       16,      16,   0 },
    { "fixup_OR1K_PLT26",        8,      26,   MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_OR1K_GOTOFF_HI16", 16,      16,   0 },
    { "fixup_OR1K_GOTOFF_LO16", 16,      16,   0 },
    { "fixup_OR1K_COPY",         0,      32,   0 },
    { "fixup_OR1K_GLOB_DAT",     0,      32,   0 },
    { "fixup_OR1K_JMP_SLOT",     0,      32,   0 },
    { "fixup_OR1K_RELATIVE",     0,      32,   0 }
  };

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
         "Invalid kind!");

  int FixupId = Kind - FirstTargetFixupKind;
  return IsLittleEndian ? LittleEndianInfos[FixupId] : BigEndianInfos[FixupId];
}

MCAsmBackend *llvm::createOR1KleAsmBackend(const Target &T,
                                           const MCRegisterInfo &MRI,
                                           StringRef TT, StringRef CPU) {
  Triple TheTriple(TT);

  if (TheTriple.isOSDarwin())
    assert(0 && "Mac not supported on OR1K");

  if (TheTriple.isOSWindows())
    assert(0 && "Windows not supported on OR1K");

  return new OR1KAsmBackend(T, Triple(TT).getOS(), /*IsLittleEndian*/ true);
}

MCAsmBackend *llvm::createOR1KbeAsmBackend(const Target &T,
                                           const MCRegisterInfo &MRI,
                                           StringRef TT, StringRef CPU) {
  Triple TheTriple(TT);

  if (TheTriple.isOSDarwin())
    assert(0 && "Mac not supported on OR1K");

  if (TheTriple.isOSWindows())
    assert(0 && "Windows not supported on OR1K");

  return new OR1KAsmBackend(T, Triple(TT).getOS(), /*IsLittleEndian*/ false);
}
