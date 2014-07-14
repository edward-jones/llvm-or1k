//===-- OR1KMCExpr.cpp - OR1K specific MC expression classes ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "OR1KMCExpr.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"

#define DEBUG_TYPE "or1k-mcexpr"

using namespace llvm;

const OR1KMCExpr *OR1KMCExpr::Create(VariantKind VK, const MCExpr *Expr,
                                     MCContext &Ctx) {
  return new (Ctx) OR1KMCExpr(VK, Expr);
}

StringRef OR1KMCExpr::getVariantKindName(OR1KMCExpr::VariantKind VK) {
  switch (VK) {
  default:
    return "";
  case VK_OR1K_ABS_HI16:
    return "hi";
  case VK_OR1K_ABS_LO16:
    return "lo";
  case VK_OR1K_PLT26:
    return "plt";
  case VK_OR1K_GOT16:
    return "got";
  case VK_OR1K_GOTPC_HI16:
    return "gotpchi";
  case VK_OR1K_GOTPC_LO16:
    return "gotpclo";
  case VK_OR1K_GOTOFF_HI16:
    return "gotoffhi";
  case VK_OR1K_GOTOFF_LO16:
    return "gotofflo";
  }
}

OR1KMCExpr::VariantKind OR1KMCExpr::getVariantKindForName(StringRef Name) {
  return llvm::StringSwitch<OR1KMCExpr::VariantKind>(Name)
      .Case("hi", VK_OR1K_ABS_HI16)
      .Case("HI", VK_OR1K_ABS_HI16)
      .Case("lo", VK_OR1K_ABS_LO16)
      .Case("LO", VK_OR1K_ABS_LO16)
      .Case("plt", VK_OR1K_PLT26)
      .Case("PLT", VK_OR1K_PLT26)
      .Case("got", VK_OR1K_GOT16)
      .Case("GOT", VK_OR1K_GOT16)
      .Case("gotpchi", VK_OR1K_GOTPC_HI16)
      .Case("GOTPCHI", VK_OR1K_GOTPC_HI16)
      .Case("gotpclo", VK_OR1K_GOTPC_LO16)
      .Case("GOTPCLO", VK_OR1K_GOTPC_LO16)
      .Case("gotoffhi", VK_OR1K_GOTOFF_HI16)
      .Case("GOTOFFHI", VK_OR1K_GOTOFF_HI16)
      .Case("gotofflo", VK_OR1K_GOTOFF_LO16)
      .Case("GOTOFFLO", VK_OR1K_GOTOFF_LO16)
      .Default(VK_Invalid);
}

static bool isVariantKindExplicit(OR1KMCExpr::VariantKind VK) {
  switch (VK) {
  default:
    return false;
  case OR1KMCExpr::VK_OR1K_ABS_HI16:
  case OR1KMCExpr::VK_OR1K_ABS_LO16:
  case OR1KMCExpr::VK_OR1K_PLT26:
  case OR1KMCExpr::VK_OR1K_GOT16:
  case OR1KMCExpr::VK_OR1K_GOTPC_HI16:
  case OR1KMCExpr::VK_OR1K_GOTPC_LO16:
  case OR1KMCExpr::VK_OR1K_GOTOFF_HI16:
  case OR1KMCExpr::VK_OR1K_GOTOFF_LO16:
    return true;
  }
}

void OR1KMCExpr::PrintImpl(raw_ostream &OS) const {
  bool NeedParens = isVariantKindExplicit(Variant) || isa<MCBinaryExpr>(Expr);

  OS << getVariantKindName(Variant);

  if (NeedParens)
    OS << '(';
  Expr->print(OS);
  if (NeedParens)
    OS << ')';
}

bool OR1KMCExpr::EvaluateAsRelocatableImpl(MCValue &Res,
                                           const MCAsmLayout *Layout) const {
  return getSubExpr()->EvaluateAsRelocatable(Res, Layout);
}

void OR1KMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}
