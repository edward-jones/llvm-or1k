//===-- OR1KBaseInfo.h - Top level definitions for OR1K MC ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the OR1K target useful for the compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//

#ifndef OR1KBASEINFO_H
#define OR1KBASEINFO_H

#include "OR1KMCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
/// \brief This namespace holds all of the target specific flags that
/// instruction info tracks.
namespace OR1KII {
/// Target Operand Flag enum.
enum TOF {
  MO_NO_FLAG,

  // High/Low part of an absolute symbol address.
  MO_ABS_HI16,
  MO_ABS_LO16,

  // 16bit offset from the base of the GOT for a given symbol.
  MO_GOT16,

  // High/Low part of the offset from the base of the GOT for a given symbol.
  MO_GOTOFF_HI16,
  MO_GOTOFF_LO16,

  // High/Low part of the offset to the GOT entry for a given symbol from the
  // the current code location.
  MO_GOTPC_HI16,
  MO_GOTPC_LO16,

  // 26bit offset to the PLT entry of a given symbol from the current code
  // location.
  MO_PLT26
};
}
}

#endif
