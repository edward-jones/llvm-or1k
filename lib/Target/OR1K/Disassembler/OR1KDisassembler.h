#ifndef OR1KDISASSEMBLER_H
#define OR1KDISASSEMBLER_H

#include "llvm/MC/MCDisassembler.h"

namespace llvm {

class MCInst;
class MemoryObject;
class raw_ostream;

class OR1KDisassembler : public MCDisassembler {
  bool IsLittleEndian;

public:
  OR1KDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                   bool IsLittleEndian)
    : MCDisassembler(STI, Ctx), IsLittleEndian(IsLittleEndian) {}

  ~OR1KDisassembler() { }

  MCDisassembler::DecodeStatus getInstruction(MCInst &instr,
                      uint64_t &size,
                      const MemoryObject &region,
                      uint64_t address,
                      raw_ostream &vStream,
                      raw_ostream &cStream) const;
};

} // namespace llvm

#endif
