//===-- OR1KMCExpr.h - OR1K specific MC expression classes ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef OR1KMCEXPR_H
#define OR1KMCEXPR_H

#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCValue.h"

namespace llvm {
class OR1KMCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_None,
    VK_OR1K_ABS_HI16,
    VK_OR1K_ABS_LO16,
    VK_OR1K_REL26,
    VK_OR1K_PLT26,
    VK_OR1K_GOT16,
    VK_OR1K_GOTPC_HI16,
    VK_OR1K_GOTPC_LO16,
    VK_OR1K_GOTOFF_HI16,
    VK_OR1K_GOTOFF_LO16,

    VK_Invalid
  };

public:
  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

public:
  static const OR1KMCExpr *Create(VariantKind VK, const MCExpr *Expr,
                                  MCContext &Ctx);

  VariantKind getVariantKind() const { return Variant; }

  const MCExpr *getSubExpr() const { return Expr; }

  void PrintImpl(raw_ostream &OS) const override;

  bool EvaluateAsRelocatableImpl(MCValue &, const MCAsmLayout *) const override;

  void AddValueSymbols(MCAssembler *Asm) const override;

  const MCSection *FindAssociatedSection() const override {
    return getSubExpr()->FindAssociatedSection();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &A) const override {}

public:
  static VariantKind getVariantKindForName(StringRef Name);
  static StringRef getVariantKindName(VariantKind VK);

private:
  explicit OR1KMCExpr(VariantKind VK, const MCExpr *Expr)
      : Variant(VK), Expr(Expr) {}

private:
  VariantKind Variant;
  const MCExpr *Expr;
};
}

#endif
