//===-- OR1KAsmParser.cpp - Parse OR1K assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/OR1KMCExpr.h"
#include "MCTargetDesc/OR1KMCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

namespace {
class OR1KOperand;

class OR1KAsmParser : public MCTargetAsmParser {
public:
  typedef SmallVectorImpl<MCParsedAsmOperand*> ParsedOperandsVector;

public:
  OR1KAsmParser(MCSubtargetInfo &STI, MCAsmParser &P,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
   : STI(STI) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, ParsedOperandsVector &Operands) override;

  bool ParseDirective(AsmToken DirectiveID) override;

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               ParsedOperandsVector &Operands,
                               MCStreamer &Out, unsigned &ErrorInfo,
                               bool MatchingInlineAsm) override;

  #define GET_ASSEMBLER_HEADER
  #include "OR1KGenAsmMatcher.inc"

private:
  bool parseOperand(ParsedOperandsVector &Operands, StringRef Name);
  OperandMatchResultTy parseMemOperand(ParsedOperandsVector &Operands);
  OperandMatchResultTy parseJumpTargetOperand(ParsedOperandsVector &Operands);

  OperandMatchResultTy parseRegister(ParsedOperandsVector &Operands,
                                     StringRef Name);
  OperandMatchResultTy parseImmediate(ParsedOperandsVector &Operands,
                                      StringRef Name);

  bool matchImmediate(const MCExpr *&Expr, SMLoc &EndLoc);
  bool matchJumpTarget(const MCExpr *&Expr, SMLoc &EndLoc);

private:
  MCSubtargetInfo &STI;
};

/// OR1KOperand - Instances of this class represented a parsed machine
/// instruction
class OR1KOperand : public MCParsedAsmOperand {
public:
  enum KindTy {
    Token,
    Register,
    Immediate,
    Memory,
    JumpTarget
  };

private:
  struct MemAddr {
    unsigned BaseReg;
    const MCExpr *Offset;
  };

  union ValueTy {
    unsigned Reg;
    const MCExpr *Imm;
    MemAddr Mem;
    StringRef Tok;

    ValueTy() {}
  };

public:
  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isToken() const override { return Kind == Token; }
  bool isMem() const override { return Kind == Memory; }
  bool isJumpTarget() const { return Kind == JumpTarget; }

  unsigned getReg() const override {
    assert(Kind == Register && "Invalid type access!");
    return Value.Reg;
  }

  const MCExpr *getImm() const {
    assert(Kind == Immediate && "Invalid type access!");
    return Value.Imm;
  }

  StringRef getToken() const {
    assert(Kind == Token && "Invalid type access!");
    return Value.Tok;
  }

  MemAddr getMem() const {
    assert(Kind == Memory && "Invalid type access!");
    return Value.Mem;
  }

  const MCExpr *getJumpTarget() const {
    assert(Kind == JumpTarget && "Invalid type access!");
    return Value.Imm;
  }

  void print(raw_ostream &OS) const override {}

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCExpr *E = getImm();

    int64_t Value;
    if (E->EvaluateAsAbsolute(Value))
      Inst.addOperand(MCOperand::CreateImm(Value));
    else
      Inst.addOperand(MCOperand::CreateExpr(E));
  }

  void addMemOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");
    MemAddr Mem = getMem();
    
    Inst.addOperand(MCOperand::CreateReg(Mem.BaseReg));

    int64_t OffsetValue;
    if (Mem.Offset->EvaluateAsAbsolute(OffsetValue))
      Inst.addOperand(MCOperand::CreateImm(OffsetValue));
    else
      Inst.addOperand(MCOperand::CreateExpr(Mem.Offset));
  }

  void addJumpTargetOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCExpr *E = getJumpTarget();

    int64_t Value;
    if (E->EvaluateAsAbsolute(Value))
      Inst.addOperand(MCOperand::CreateImm(Value));
    else
      Inst.addOperand(MCOperand::CreateExpr(E));
  }

public:
  static OR1KOperand *CreateToken(StringRef Value, SMLoc StartLoc,
                                  SMLoc EndLoc) {
    OR1KOperand *Op = new OR1KOperand(Token);
    Op->Value.Tok = Value;
    Op->StartLoc = StartLoc;
    Op->EndLoc = EndLoc;
    return Op;
  }

  static OR1KOperand *CreateReg(unsigned Reg, SMLoc StartLoc,
                                SMLoc EndLoc) {
    OR1KOperand *Op = new OR1KOperand(Register);
    Op->Value.Reg = Reg;
    Op->StartLoc = StartLoc;
    Op->EndLoc = EndLoc;
    return Op;
  }

  static OR1KOperand *CreateImm(const MCExpr *Exp, SMLoc StartLoc,
                                SMLoc EndLoc) {
    OR1KOperand *Op = new OR1KOperand(Immediate);
    Op->Value.Imm = Exp;
    Op->StartLoc = StartLoc;
    Op->EndLoc = EndLoc;
    return Op;
  }

  static OR1KOperand *CreateMem(unsigned BaseReg, const MCExpr *Offset,
                                SMLoc StartLoc, SMLoc EndLoc) {
    OR1KOperand *Op = new OR1KOperand(Memory);
    Op->Value.Mem.BaseReg = BaseReg;
    Op->Value.Mem.Offset = Offset;
    Op->StartLoc = StartLoc;
    Op->EndLoc = EndLoc;
    return Op;
  }

  static OR1KOperand *CreateJumpTarget(const MCExpr *Exp, SMLoc StartLoc,
                                        SMLoc EndLoc) {
    OR1KOperand *Op = new OR1KOperand(JumpTarget);
    Op->Value.Imm = Exp;
    Op->StartLoc = StartLoc;
    Op->EndLoc = EndLoc;
    return Op;
  }

private:
  OR1KOperand(KindTy K) : Kind(K) {}

private:
  KindTy Kind;
  ValueTy Value;
  SMLoc StartLoc, EndLoc;
};

} // end anonymous namespace.

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "OR1KGenAsmMatcher.inc"

bool OR1KAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  if (Tok.getKind() != AsmToken::Identifier)
    return true;

  RegNo = MatchRegisterName(Tok.getString().lower());

  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();

  return RegNo == 0;
}

bool OR1KAsmParser::matchImmediate(const MCExpr *&Expr, SMLoc &EndLoc) {
  const AsmToken &Tok = getLexer().getTok();
  SMLoc StartLoc = Tok.getLoc();
  MCContext &Ctx = getParser().getContext();

  OR1KMCExpr::VariantKind VK = OR1KMCExpr::VK_Invalid;

  if (Tok.getKind() == AsmToken::Identifier)
    VK = OR1KMCExpr::getVariantKindForName(Tok.getString());

  // Check for relocation variant name
  if (Tok.getKind() == AsmToken::Identifier &&
      VK != OR1KMCExpr::VK_Invalid) {
    Lex();

    if (Tok.getKind() != AsmToken::LParen)
      return true;

    Lex();

    if (getParser().parseExpression(Expr))
      return true;

    if (Tok.getKind() != AsmToken::RParen)
      return true;

    Lex();

    EndLoc = Tok.getLoc();

    switch (VK) {
    default: return Error(StartLoc, "invalid relocation for immediate value");
    case OR1KMCExpr::VK_OR1K_ABS_HI16:
    case OR1KMCExpr::VK_OR1K_ABS_LO16:
    case OR1KMCExpr::VK_OR1K_GOT16:
    case OR1KMCExpr::VK_OR1K_GOTPC_HI16:
    case OR1KMCExpr::VK_OR1K_GOTPC_LO16:
    case OR1KMCExpr::VK_OR1K_GOTOFF_HI16:
    case OR1KMCExpr::VK_OR1K_GOTOFF_LO16:
      break;
    }

    Expr = OR1KMCExpr::Create(VK, Expr, Ctx);

    return false;
  }

  if (getParser().parseExpression(Expr, EndLoc))
    return true;

  int64_t Value;
  if (!Expr->EvaluateAsAbsolute(Value))
    return Error(StartLoc, "missing relocation for symbolic expression");

  if (!isUInt<16>(abs64(Value)))
    return Error(StartLoc, "expression value out of bounds");

  Expr = MCConstantExpr::Create(Value, Ctx);
  return false;
}

bool OR1KAsmParser::matchJumpTarget(const MCExpr *&Expr, SMLoc &EndLoc) {
  const AsmToken &Tok = getLexer().getTok();
  SMLoc StartLoc = Tok.getLoc();
  MCContext &Ctx = getParser().getContext();

  OR1KMCExpr::VariantKind VK = OR1KMCExpr::VK_Invalid;

  if (Tok.getKind() == AsmToken::Identifier)
    VK = OR1KMCExpr::getVariantKindForName(Tok.getString());

  // Check for relocation variant name
  if (Tok.getKind() == AsmToken::Identifier &&
      VK != OR1KMCExpr::VK_Invalid) {
    Lex();

    if (Tok.getKind() != AsmToken::LParen)
      return true;

    Lex();

    if (getParser().parseExpression(Expr))
      return true;

    if (Tok.getKind() != AsmToken::RParen)
      return true;

    Lex();

    EndLoc = Tok.getLoc();

    if (VK != OR1KMCExpr::VK_OR1K_PLT26)
      return Error(StartLoc, "invalid relocation for jump target value");

    Expr = OR1KMCExpr::Create(VK, Expr, Ctx);

    return false;
  }

  if (getParser().parseExpression(Expr, EndLoc))
    return true;

  int64_t Value;
  if (!Expr->EvaluateAsAbsolute(Value)) {
    Expr = OR1KMCExpr::Create(OR1KMCExpr::VK_OR1K_REL26, Expr, Ctx);
    return false;
  }

  if (!isShiftedInt<26, 2>(Value))
    return Error(StartLoc, "expression value out of bounds");

  Expr = MCConstantExpr::Create(Value, Ctx);
  return false;
}

OR1KAsmParser::OperandMatchResultTy
OR1KAsmParser::parseJumpTargetOperand(ParsedOperandsVector &Operands) {
  const AsmToken &Tok = getLexer().getTok();
  SMLoc StartLoc = Tok.getLoc();

  const MCExpr *Expr = 0;
  SMLoc EndLoc;
  if (matchJumpTarget(Expr, EndLoc))
    return MatchOperand_NoMatch;

  Operands.push_back(OR1KOperand::CreateJumpTarget(Expr, StartLoc, EndLoc));

  return MatchOperand_Success;
}

OR1KAsmParser::OperandMatchResultTy
OR1KAsmParser::parseMemOperand(ParsedOperandsVector &Operands) {
  const AsmToken &Tok = getLexer().getTok();
  SMLoc StartLoc = Tok.getLoc();

  const MCExpr *Offset = 0;
  SMLoc OffsetEndLoc;
  if (matchImmediate(Offset, OffsetEndLoc))
    return MatchOperand_NoMatch;

  assert(Offset && "Invalid offset!");

  if (Tok.getKind() != AsmToken::LParen)
    return MatchOperand_NoMatch;

  Lex();

  unsigned Reg;
  SMLoc StartRegLoc;
  SMLoc EndRegLoc;
  if (ParseRegister(Reg, StartRegLoc, EndRegLoc))
    return MatchOperand_NoMatch;

  Lex();

  if (Tok.getKind() != AsmToken::RParen)
    return MatchOperand_ParseFail;

  SMLoc EndLoc = Tok.getEndLoc();
  Operands.push_back(OR1KOperand::CreateMem(Reg, Offset, StartLoc, EndLoc));

  Lex();

  return MatchOperand_Success;
}

OR1KAsmParser::OperandMatchResultTy
OR1KAsmParser::parseRegister(ParsedOperandsVector &Operands, StringRef Name) {
  unsigned RegNo;
  SMLoc StartLoc;
  SMLoc EndLoc;

  if (ParseRegister(RegNo, StartLoc, EndLoc))
    return MatchOperand_NoMatch;

  Operands.push_back(OR1KOperand::CreateReg(RegNo, StartLoc, EndLoc));

  Lex();
  return MatchOperand_Success;
}

OR1KAsmParser::OperandMatchResultTy
OR1KAsmParser::parseImmediate(ParsedOperandsVector &Operands, StringRef Name) {
  const AsmToken &Tok = getLexer().getTok();

  SMLoc StartLoc = Tok.getLoc();
  SMLoc EndLoc = Tok.getEndLoc();

  switch (Tok.getKind()) {
  case AsmToken::Integer:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::LParen:
  case AsmToken::Identifier: {
    const MCExpr *Exp;
    if (matchImmediate(Exp, EndLoc))
      break;

    Operands.push_back(OR1KOperand::CreateImm(Exp, StartLoc, EndLoc));
    return MatchOperand_Success;
  }
  default:
    break;
  }
  return MatchOperand_NoMatch;
}

#define CHECK_OP_MATCH(Res)       \
  do {                           \
    switch (Res) {               \
    case MatchOperand_Success:   \
      return false;              \
    case MatchOperand_ParseFail: \
      return false;              \
    case MatchOperand_NoMatch:   \
      break;                     \
    }                            \
  } while (0)

bool OR1KAsmParser::parseOperand(ParsedOperandsVector &Operands,
                                 StringRef Name) {
  // Try custom parsers first.
  CHECK_OP_MATCH(MatchOperandParserImpl(Operands, Name));

  // Try to parse registers.
  CHECK_OP_MATCH(parseRegister(Operands, Name));

  // Try to parse immediates.
  CHECK_OP_MATCH(parseImmediate(Operands, Name));

  return true;
}

#undef CHECK_OP_MATCH

bool OR1KAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                     StringRef Name, SMLoc NameLoc,
                                     ParsedOperandsVector &Operands) {
  // Check if we have valid mnemonic
  if (!mnemonicIsValid(Name, 0)) {
    getParser().eatToEndOfStatement();
    return Error(NameLoc, "Unknown instruction");
  }
  // First operand in MCInst is instruction mnemonic.
  Operands.push_back(OR1KOperand::CreateToken(Name, NameLoc, NameLoc));

  SMLoc OpLoc;

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    OpLoc = getLexer().getLoc();
    // Read the first operand.
    if (parseOperand(Operands, Name)) {
      getParser().eatToEndOfStatement();
      return Error(OpLoc, "illegal operand");
    }

    while (getLexer().is(AsmToken::Comma)) {
      getParser().Lex(); // Eat the comma.
      OpLoc = getLexer().getLoc();
      // Parse and remember the operand.
      if (parseOperand(Operands, Name)) {
        getParser().eatToEndOfStatement();
        return Error(OpLoc, "illegal operand");
      }
    }
  }
  OpLoc = getLexer().getLoc();
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    getParser().eatToEndOfStatement();
    return Error(OpLoc, "unexpected token in argument list");
  }
  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

bool OR1KAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                            ParsedOperandsVector &Operands,
                                            MCStreamer &Out,
                                            unsigned &ErrorInfo,
                                            bool MatchingInlineAsm) {
  MCInst Inst;

  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success:
    Out.EmitInstruction(Inst, STI);
    return false;
  case Match_MissingFeature:
    return Error(IDLoc, "Instruction use requires option to be enabled");
  case Match_MnemonicFail:
    return Error(IDLoc, "Unrecognized instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "Too few operands for instruction");

      ErrorLoc = Operands[ErrorInfo]->getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }
    return Error(IDLoc, "Invalid operand for instruction");
  }
  default: break;
  }
  return true;
}

bool OR1KAsmParser::ParseDirective(AsmToken DirectiveID) {
  return true;
}

extern "C" void LLVMInitializeOR1KAsmParser() {
  RegisterMCAsmParser<OR1KAsmParser> X(TheOR1KbeTarget);
  RegisterMCAsmParser<OR1KAsmParser> Y(TheOR1KleTarget);
}
