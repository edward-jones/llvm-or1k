//===- lib/MC/MCStreamer.cpp - Streaming Machine Code Output --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCStreamer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
using namespace llvm;

// Pin the vtables to this file.
MCTargetStreamer::~MCTargetStreamer() {}

MCTargetStreamer::MCTargetStreamer(MCStreamer &S) : Streamer(S) {
  S.setTargetStreamer(this);
}

void MCTargetStreamer::emitLabel(MCSymbol *Symbol) {}

void MCTargetStreamer::finish() {}

void MCTargetStreamer::emitAssignment(MCSymbol *Symbol, const MCExpr *Value) {}

MCStreamer::MCStreamer(MCContext &Ctx)
    : Context(Ctx), CurrentWinFrameInfo(nullptr) {
  SectionStack.push_back(std::pair<MCSectionSubPair, MCSectionSubPair>());
}

MCStreamer::~MCStreamer() {
  for (unsigned i = 0; i < getNumWinFrameInfos(); ++i)
    delete WinFrameInfos[i];
}

void MCStreamer::reset() {
  for (unsigned i = 0; i < getNumWinFrameInfos(); ++i)
    delete WinFrameInfos[i];
  WinFrameInfos.clear();
  CurrentWinFrameInfo = nullptr;
  SectionStack.clear();
  SectionStack.push_back(std::pair<MCSectionSubPair, MCSectionSubPair>());
}

const MCExpr *MCStreamer::BuildSymbolDiff(MCContext &Context,
                                          const MCSymbol *A,
                                          const MCSymbol *B) {
  MCSymbolRefExpr::VariantKind Variant = MCSymbolRefExpr::VK_None;
  const MCExpr *ARef =
    MCSymbolRefExpr::Create(A, Variant, Context);
  const MCExpr *BRef =
    MCSymbolRefExpr::Create(B, Variant, Context);
  const MCExpr *AddrDelta =
    MCBinaryExpr::Create(MCBinaryExpr::Sub, ARef, BRef, Context);
  return AddrDelta;
}

const MCExpr *MCStreamer::ForceExpAbs(const MCExpr* Expr) {
  assert(!isa<MCSymbolRefExpr>(Expr));
  if (Context.getAsmInfo()->hasAggressiveSymbolFolding())
    return Expr;

  MCSymbol *ABS = Context.CreateTempSymbol();
  EmitAssignment(ABS, Expr);
  return MCSymbolRefExpr::Create(ABS, Context);
}

raw_ostream &MCStreamer::GetCommentOS() {
  // By default, discard comments.
  return nulls();
}

void MCStreamer::emitRawComment(const Twine &T, bool TabPrefix) {}

void MCStreamer::generateCompactUnwindEncodings(MCAsmBackend *MAB) {
  for (auto &FI : DwarfFrameInfos)
    FI.CompactUnwindEncoding =
        (MAB ? MAB->generateCompactUnwindEncoding(FI.Instructions) : 0);
}

void MCStreamer::EmitDwarfSetLineAddr(int64_t LineDelta,
                                      const MCSymbol *Label, int PointerSize) {
  // emit the sequence to set the address
  EmitIntValue(dwarf::DW_LNS_extended_op, 1);
  EmitULEB128IntValue(PointerSize + 1);
  EmitIntValue(dwarf::DW_LNE_set_address, 1);
  EmitSymbolValue(Label, PointerSize);

  // emit the sequence for the LineDelta (from 1) and a zero address delta.
  MCDwarfLineAddr::Emit(this, LineDelta, 0);
}

/// EmitIntValue - Special case of EmitValue that avoids the client having to
/// pass in a MCExpr for constant integers.
void MCStreamer::EmitIntValue(uint64_t Value, unsigned Size) {
  assert(Size <= 8 && "Invalid size");
  assert((isUIntN(8 * Size, Value) || isIntN(8 * Size, Value)) &&
         "Invalid size");
  char buf[8];
  const bool isLittleEndian = Context.getAsmInfo()->isLittleEndian();
  for (unsigned i = 0; i != Size; ++i) {
    unsigned index = isLittleEndian ? i : (Size - i - 1);
    buf[i] = uint8_t(Value >> (index * 8));
  }
  EmitBytes(StringRef(buf, Size));
}

/// EmitULEB128Value - Special case of EmitULEB128Value that avoids the
/// client having to pass in a MCExpr for constant integers.
void MCStreamer::EmitULEB128IntValue(uint64_t Value, unsigned Padding) {
  SmallString<128> Tmp;
  raw_svector_ostream OSE(Tmp);
  encodeULEB128(Value, OSE, Padding);
  EmitBytes(OSE.str());
}

/// EmitSLEB128Value - Special case of EmitSLEB128Value that avoids the
/// client having to pass in a MCExpr for constant integers.
void MCStreamer::EmitSLEB128IntValue(int64_t Value) {
  SmallString<128> Tmp;
  raw_svector_ostream OSE(Tmp);
  encodeSLEB128(Value, OSE);
  EmitBytes(OSE.str());
}

void MCStreamer::EmitAbsValue(const MCExpr *Value, unsigned Size) {
  const MCExpr *ABS = ForceExpAbs(Value);
  EmitValue(ABS, Size);
}


void MCStreamer::EmitValue(const MCExpr *Value, unsigned Size,
                           const SMLoc &Loc) {
  EmitValueImpl(Value, Size, Loc);
}

void MCStreamer::EmitSymbolValue(const MCSymbol *Sym, unsigned Size) {
  EmitValueImpl(MCSymbolRefExpr::Create(Sym, getContext()), Size);
}

void MCStreamer::EmitGPRel64Value(const MCExpr *Value) {
  report_fatal_error("unsupported directive in streamer");
}

void MCStreamer::EmitGPRel32Value(const MCExpr *Value) {
  report_fatal_error("unsupported directive in streamer");
}

/// EmitFill - Emit NumBytes bytes worth of the value specified by
/// FillValue.  This implements directives such as '.space'.
void MCStreamer::EmitFill(uint64_t NumBytes, uint8_t FillValue) {
  const MCExpr *E = MCConstantExpr::Create(FillValue, getContext());
  for (uint64_t i = 0, e = NumBytes; i != e; ++i)
    EmitValue(E, 1);
}

/// The implementation in this class just redirects to EmitFill.
void MCStreamer::EmitZeros(uint64_t NumBytes) {
  EmitFill(NumBytes, 0);
}

unsigned MCStreamer::EmitDwarfFileDirective(unsigned FileNo,
                                            StringRef Directory,
                                            StringRef Filename, unsigned CUID) {
  return getContext().GetDwarfFile(Directory, Filename, FileNo, CUID);
}

void MCStreamer::EmitDwarfLocDirective(unsigned FileNo, unsigned Line,
                                       unsigned Column, unsigned Flags,
                                       unsigned Isa,
                                       unsigned Discriminator,
                                       StringRef FileName) {
  getContext().setCurrentDwarfLoc(FileNo, Line, Column, Flags, Isa,
                                  Discriminator);
}

MCSymbol *MCStreamer::getDwarfLineTableSymbol(unsigned CUID) {
  MCDwarfLineTable &Table = getContext().getMCDwarfLineTable(CUID);
  if (!Table.getLabel()) {
    StringRef Prefix = Context.getAsmInfo()->getPrivateGlobalPrefix();
    Table.setLabel(
        Context.GetOrCreateSymbol(Prefix + "line_table_start" + Twine(CUID)));
  }
  return Table.getLabel();
}

MCDwarfFrameInfo *MCStreamer::getCurrentDwarfFrameInfo() {
  if (DwarfFrameInfos.empty())
    return nullptr;
  return &DwarfFrameInfos.back();
}

void MCStreamer::EnsureValidDwarfFrame() {
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  if (!CurFrame || CurFrame->End)
    report_fatal_error("No open frame");
}

void MCStreamer::EmitEHSymAttributes(const MCSymbol *Symbol,
                                     MCSymbol *EHSymbol) {
}

void MCStreamer::InitSections() {
  SwitchSection(getContext().getObjectFileInfo()->getTextSection());
}

void MCStreamer::AssignSection(MCSymbol *Symbol, const MCSection *Section) {
  if (Section)
    Symbol->setSection(*Section);
  else
    Symbol->setUndefined();

  // As we emit symbols into a section, track the order so that they can
  // be sorted upon later. Zero is reserved to mean 'unemitted'.
  SymbolOrdering[Symbol] = 1 + SymbolOrdering.size();
}

void MCStreamer::EmitLabel(MCSymbol *Symbol) {
  assert(!Symbol->isVariable() && "Cannot emit a variable symbol!");
  assert(getCurrentSection().first && "Cannot emit before setting section!");
  AssignSection(Symbol, getCurrentSection().first);

  MCTargetStreamer *TS = getTargetStreamer();
  if (TS)
    TS->emitLabel(Symbol);
}

void MCStreamer::EmitCompactUnwindEncoding(uint32_t CompactUnwindEncoding) {
  EnsureValidDwarfFrame();
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->CompactUnwindEncoding = CompactUnwindEncoding;
}

void MCStreamer::EmitCFISections(bool EH, bool Debug) {
  assert(EH || Debug);
}

void MCStreamer::EmitCFIStartProc(bool IsSimple) {
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  if (CurFrame && !CurFrame->End)
    report_fatal_error("Starting a frame before finishing the previous one!");

  MCDwarfFrameInfo Frame;
  Frame.IsSimple = IsSimple;
  EmitCFIStartProcImpl(Frame);

  DwarfFrameInfos.push_back(Frame);
}

void MCStreamer::EmitCFIStartProcImpl(MCDwarfFrameInfo &Frame) {
}

void MCStreamer::EmitCFIEndProc() {
  EnsureValidDwarfFrame();
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  EmitCFIEndProcImpl(*CurFrame);
}

void MCStreamer::EmitCFIEndProcImpl(MCDwarfFrameInfo &Frame) {
  // Put a dummy non-null value in Frame.End to mark that this frame has been
  // closed.
  Frame.End = (MCSymbol *) 1;
}

MCSymbol *MCStreamer::EmitCFICommon() {
  EnsureValidDwarfFrame();
  MCSymbol *Label = getContext().CreateTempSymbol();
  EmitLabel(Label);
  return Label;
}

void MCStreamer::EmitCFIDefCfa(int64_t Register, int64_t Offset) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createDefCfa(Label, Register, Offset);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIDefCfaOffset(int64_t Offset) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createDefCfaOffset(Label, Offset);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIAdjustCfaOffset(int64_t Adjustment) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createAdjustCfaOffset(Label, Adjustment);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIDefCfaRegister(int64_t Register) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createDefCfaRegister(Label, Register);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIOffset(int64_t Register, int64_t Offset) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createOffset(Label, Register, Offset);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIRelOffset(int64_t Register, int64_t Offset) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createRelOffset(Label, Register, Offset);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIPersonality(const MCSymbol *Sym,
                                    unsigned Encoding) {
  EnsureValidDwarfFrame();
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Personality = Sym;
  CurFrame->PersonalityEncoding = Encoding;
}

void MCStreamer::EmitCFILsda(const MCSymbol *Sym, unsigned Encoding) {
  EnsureValidDwarfFrame();
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Lsda = Sym;
  CurFrame->LsdaEncoding = Encoding;
}

void MCStreamer::EmitCFIRememberState() {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction = MCCFIInstruction::createRememberState(Label);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIRestoreState() {
  // FIXME: Error if there is no matching cfi_remember_state.
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction = MCCFIInstruction::createRestoreState(Label);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFISameValue(int64_t Register) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createSameValue(Label, Register);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIRestore(int64_t Register) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createRestore(Label, Register);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIEscape(StringRef Values) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction = MCCFIInstruction::createEscape(Label, Values);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFISignalFrame() {
  EnsureValidDwarfFrame();
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->IsSignalFrame = true;
}

void MCStreamer::EmitCFIUndefined(int64_t Register) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createUndefined(Label, Register);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIRegister(int64_t Register1, int64_t Register2) {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createRegister(Label, Register1, Register2);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EmitCFIWindowSave() {
  MCSymbol *Label = EmitCFICommon();
  MCCFIInstruction Instruction =
    MCCFIInstruction::createWindowSave(Label);
  MCDwarfFrameInfo *CurFrame = getCurrentDwarfFrameInfo();
  CurFrame->Instructions.push_back(Instruction);
}

void MCStreamer::EnsureValidWinFrameInfo() {
  if (!CurrentWinFrameInfo || CurrentWinFrameInfo->End)
    report_fatal_error("No open Win64 EH frame function!");
}

void MCStreamer::EmitWinCFIStartProc(const MCSymbol *Symbol) {
  if (CurrentWinFrameInfo && !CurrentWinFrameInfo->End)
    report_fatal_error("Starting a function before ending the previous one!");
  MCWinFrameInfo *Frame = new MCWinFrameInfo;
  Frame->Begin = getContext().CreateTempSymbol();
  Frame->Function = Symbol;
  EmitLabel(Frame->Begin);
  WinFrameInfos.push_back(Frame);
  CurrentWinFrameInfo = WinFrameInfos.back();
}

void MCStreamer::EmitWinCFIEndProc() {
  EnsureValidWinFrameInfo();
  if (CurrentWinFrameInfo->ChainedParent)
    report_fatal_error("Not all chained regions terminated!");
  CurrentWinFrameInfo->End = getContext().CreateTempSymbol();
  EmitLabel(CurrentWinFrameInfo->End);
}

void MCStreamer::EmitWinCFIStartChained() {
  EnsureValidWinFrameInfo();
  MCWinFrameInfo *Frame = new MCWinFrameInfo;
  Frame->Begin = getContext().CreateTempSymbol();
  Frame->Function = CurrentWinFrameInfo->Function;
  Frame->ChainedParent = CurrentWinFrameInfo;
  EmitLabel(Frame->Begin);
  WinFrameInfos.push_back(Frame);
  CurrentWinFrameInfo = WinFrameInfos.back();
}

void MCStreamer::EmitWinCFIEndChained() {
  EnsureValidWinFrameInfo();
  if (!CurrentWinFrameInfo->ChainedParent)
    report_fatal_error("End of a chained region outside a chained region!");
  CurrentWinFrameInfo->End = getContext().CreateTempSymbol();
  EmitLabel(CurrentWinFrameInfo->End);
  CurrentWinFrameInfo = CurrentWinFrameInfo->ChainedParent;
}

void MCStreamer::EmitWinEHHandler(const MCSymbol *Sym, bool Unwind,
                                  bool Except) {
  EnsureValidWinFrameInfo();
  if (CurrentWinFrameInfo->ChainedParent)
    report_fatal_error("Chained unwind areas can't have handlers!");
  CurrentWinFrameInfo->ExceptionHandler = Sym;
  if (!Except && !Unwind)
    report_fatal_error("Don't know what kind of handler this is!");
  if (Unwind)
    CurrentWinFrameInfo->HandlesUnwind = true;
  if (Except)
    CurrentWinFrameInfo->HandlesExceptions = true;
}

void MCStreamer::EmitWinEHHandlerData() {
  EnsureValidWinFrameInfo();
  if (CurrentWinFrameInfo->ChainedParent)
    report_fatal_error("Chained unwind areas can't have handlers!");
}

void MCStreamer::EmitWinCFIPushReg(unsigned Register) {
  EnsureValidWinFrameInfo();
  MCSymbol *Label = getContext().CreateTempSymbol();
  MCWin64EHInstruction Inst(Win64EH::UOP_PushNonVol, Label, Register);
  EmitLabel(Label);
  CurrentWinFrameInfo->Instructions.push_back(Inst);
}

void MCStreamer::EmitWinCFISetFrame(unsigned Register, unsigned Offset) {
  EnsureValidWinFrameInfo();
  if (CurrentWinFrameInfo->LastFrameInst >= 0)
    report_fatal_error("Frame register and offset already specified!");
  if (Offset & 0x0F)
    report_fatal_error("Misaligned frame pointer offset!");
  if (Offset > 240)
    report_fatal_error("Frame offset must be less than or equal to 240!");
  MCSymbol *Label = getContext().CreateTempSymbol();
  MCWin64EHInstruction Inst(Win64EH::UOP_SetFPReg, Label, Register, Offset);
  EmitLabel(Label);
  CurrentWinFrameInfo->LastFrameInst = CurrentWinFrameInfo->Instructions.size();
  CurrentWinFrameInfo->Instructions.push_back(Inst);
}

void MCStreamer::EmitWinCFIAllocStack(unsigned Size) {
  EnsureValidWinFrameInfo();
  if (Size == 0)
    report_fatal_error("Allocation size must be non-zero!");
  if (Size & 7)
    report_fatal_error("Misaligned stack allocation!");
  MCSymbol *Label = getContext().CreateTempSymbol();
  MCWin64EHInstruction Inst(Label, Size);
  EmitLabel(Label);
  CurrentWinFrameInfo->Instructions.push_back(Inst);
}

void MCStreamer::EmitWinCFISaveReg(unsigned Register, unsigned Offset) {
  EnsureValidWinFrameInfo();
  if (Offset & 7)
    report_fatal_error("Misaligned saved register offset!");
  MCSymbol *Label = getContext().CreateTempSymbol();
  MCWin64EHInstruction Inst(
     Offset > 512*1024-8 ? Win64EH::UOP_SaveNonVolBig : Win64EH::UOP_SaveNonVol,
                            Label, Register, Offset);
  EmitLabel(Label);
  CurrentWinFrameInfo->Instructions.push_back(Inst);
}

void MCStreamer::EmitWinCFISaveXMM(unsigned Register, unsigned Offset) {
  EnsureValidWinFrameInfo();
  if (Offset & 0x0F)
    report_fatal_error("Misaligned saved vector register offset!");
  MCSymbol *Label = getContext().CreateTempSymbol();
  MCWin64EHInstruction Inst(
    Offset > 512*1024-16 ? Win64EH::UOP_SaveXMM128Big : Win64EH::UOP_SaveXMM128,
                            Label, Register, Offset);
  EmitLabel(Label);
  CurrentWinFrameInfo->Instructions.push_back(Inst);
}

void MCStreamer::EmitWinCFIPushFrame(bool Code) {
  EnsureValidWinFrameInfo();
  if (CurrentWinFrameInfo->Instructions.size() > 0)
    report_fatal_error("If present, PushMachFrame must be the first UOP");
  MCSymbol *Label = getContext().CreateTempSymbol();
  MCWin64EHInstruction Inst(Win64EH::UOP_PushMachFrame, Label, Code);
  EmitLabel(Label);
  CurrentWinFrameInfo->Instructions.push_back(Inst);
}

void MCStreamer::EmitWinCFIEndProlog() {
  EnsureValidWinFrameInfo();
  CurrentWinFrameInfo->PrologEnd = getContext().CreateTempSymbol();
  EmitLabel(CurrentWinFrameInfo->PrologEnd);
}

void MCStreamer::EmitCOFFSectionIndex(MCSymbol const *Symbol) {
}

void MCStreamer::EmitCOFFSecRel32(MCSymbol const *Symbol) {
}

/// EmitRawText - If this file is backed by an assembly streamer, this dumps
/// the specified string in the output .s file.  This capability is
/// indicated by the hasRawTextSupport() predicate.
void MCStreamer::EmitRawTextImpl(StringRef String) {
  errs() << "EmitRawText called on an MCStreamer that doesn't support it, "
  " something must not be fully mc'ized\n";
  abort();
}

void MCStreamer::EmitRawText(const Twine &T) {
  SmallString<128> Str;
  EmitRawTextImpl(T.toStringRef(Str));
}

void MCStreamer::EmitWindowsUnwindTables() {
  if (!getNumWinFrameInfos())
    return;

  MCWin64EHUnwindEmitter::Emit(*this);
}

void MCStreamer::Finish() {
  if (!DwarfFrameInfos.empty() && !DwarfFrameInfos.back().End)
    report_fatal_error("Unfinished frame!");

  MCTargetStreamer *TS = getTargetStreamer();
  if (TS)
    TS->finish();

  FinishImpl();
}

void MCStreamer::EmitAssignment(MCSymbol *Symbol, const MCExpr *Value) {
  visitUsedExpr(*Value);
  Symbol->setVariableValue(Value);

  MCTargetStreamer *TS = getTargetStreamer();
  if (TS)
    TS->emitAssignment(Symbol, Value);
}

void MCStreamer::visitUsedSymbol(const MCSymbol &Sym) {
}

void MCStreamer::visitUsedExpr(const MCExpr &Expr) {
  switch (Expr.getKind()) {
  case MCExpr::Target:
    cast<MCTargetExpr>(Expr).visitUsedExpr(*this);
    break;

  case MCExpr::Constant:
    break;

  case MCExpr::Binary: {
    const MCBinaryExpr &BE = cast<MCBinaryExpr>(Expr);
    visitUsedExpr(*BE.getLHS());
    visitUsedExpr(*BE.getRHS());
    break;
  }

  case MCExpr::SymbolRef:
    visitUsedSymbol(cast<MCSymbolRefExpr>(Expr).getSymbol());
    break;

  case MCExpr::Unary:
    visitUsedExpr(*cast<MCUnaryExpr>(Expr).getSubExpr());
    break;
  }
}

void MCStreamer::EmitInstruction(const MCInst &Inst,
                                 const MCSubtargetInfo &STI) {
  // Scan for values.
  for (unsigned i = Inst.getNumOperands(); i--;)
    if (Inst.getOperand(i).isExpr())
      visitUsedExpr(*Inst.getOperand(i).getExpr());
}

void MCStreamer::EmitAssemblerFlag(MCAssemblerFlag Flag) {}
void MCStreamer::EmitThumbFunc(MCSymbol *Func) {}
void MCStreamer::EmitSymbolDesc(MCSymbol *Symbol, unsigned DescValue) {}
void MCStreamer::BeginCOFFSymbolDef(const MCSymbol *Symbol) {}
void MCStreamer::EndCOFFSymbolDef() {}
void MCStreamer::EmitFileDirective(StringRef Filename) {}
void MCStreamer::EmitCOFFSymbolStorageClass(int StorageClass) {}
void MCStreamer::EmitCOFFSymbolType(int Type) {}
void MCStreamer::EmitELFSize(MCSymbol *Symbol, const MCExpr *Value) {}
void MCStreamer::EmitLocalCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                       unsigned ByteAlignment) {}
void MCStreamer::EmitTBSSSymbol(const MCSection *Section, MCSymbol *Symbol,
                                uint64_t Size, unsigned ByteAlignment) {}
void MCStreamer::ChangeSection(const MCSection *, const MCExpr *) {}
void MCStreamer::EmitWeakReference(MCSymbol *Alias, const MCSymbol *Symbol) {}
void MCStreamer::EmitBytes(StringRef Data) {}
void MCStreamer::EmitValueImpl(const MCExpr *Value, unsigned Size,
                               const SMLoc &Loc) {
  visitUsedExpr(*Value);
}
void MCStreamer::EmitULEB128Value(const MCExpr *Value) {}
void MCStreamer::EmitSLEB128Value(const MCExpr *Value) {}
void MCStreamer::EmitValueToAlignment(unsigned ByteAlignment, int64_t Value,
                                      unsigned ValueSize,
                                      unsigned MaxBytesToEmit) {}
void MCStreamer::EmitCodeAlignment(unsigned ByteAlignment,
                                   unsigned MaxBytesToEmit) {}
bool MCStreamer::EmitValueToOffset(const MCExpr *Offset, unsigned char Value) {
  return false;
}
void MCStreamer::EmitBundleAlignMode(unsigned AlignPow2) {}
void MCStreamer::EmitBundleLock(bool AlignToEnd) {}
void MCStreamer::FinishImpl() {}
void MCStreamer::EmitBundleUnlock() {}
