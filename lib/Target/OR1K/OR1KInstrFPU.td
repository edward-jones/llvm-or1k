//===-- OR1KInstrFPU.td - OR1K FPU Instruction Information -*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the OR1K FPU instruction set.
//
//===----------------------------------------------------------------------===//

def fimm   : Operand<f32>;

def HI16f32 : SDNodeXForm<fpimm, [{
  uint64_t Val = *N->getValueAPF().bitcastToAPInt().getRawData();
  return CurDAG->getTargetConstant(Val >> 16, MVT::i32);
}]>;

def LO16f32 : SDNodeXForm<fpimm, [{
  uint64_t Val = *N->getValueAPF().bitcastToAPInt().getRawData();
  return CurDAG->getTargetConstant(Val & 0xFFFFULL, MVT::i32);
}]>;

def f32lo16 : PatLeaf<(f32 fpimm), [{
  uint64_t Val = *N->getValueAPF().bitcastToAPInt().getRawData();
  return (Val & 0xFFFFULL) == Val;
}], LO16f32>;

def f32hi16 : PatLeaf<(f32 fpimm), [{
  uint64_t Val = *N->getValueAPF().bitcastToAPInt().getRawData();
  return (Val & 0xFFFF0000ULL) == Val;
}], HI16f32>;

//===----------------------------------------------------------------------===//
// ALU instructions
//===----------------------------------------------------------------------===//

class AluF<bits<8> subOp, string asmstr, SDNode OpNode, InstrItinClass itin>
  : InstFRR<subOp, (outs GPR:$rD), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rD, $rA, $rB"),
           [(set (f32 GPR:$rD), (OpNode (f32 GPR:$rA), (f32 GPR:$rB)))], itin> {
  bits<5> rD;
  bits<5> rA;
  bits<5> rB;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{15-11} = rB;
}

def ADDrrf32 : AluF<0x00, "lf.add.s", fadd, II_FADDS>;
def SUBrrf32 : AluF<0x01, "lf.sub.s", fsub, II_FSUBS>;
def MULrrf32 : AluF<0x02, "lf.mul.s", fmul, II_FMULS>;
def DIVrrf32 : AluF<0x03, "lf.div.s", fdiv, II_FDIVS>;
def REMrrf32 : AluF<0x06, "lf.rem.s", frem, II_FREMS>;

//===----------------------------------------------------------------------===//
// fp->int and int->fp conversion
//===------------------------------------------~.----------------------------===//
class ConvF<bits<8> subOp, string asmstr, ValueType DestTy, ValueType SrcTy,
            SDNode OpNode, InstrItinClass itin>
  : InstFRR<subOp, (outs GPR:$rD), (ins GPR:$rA),
           !strconcat(asmstr, "\t$rD, $rA"),
           [(set (DestTy GPR:$rD), (OpNode (SrcTy GPR:$rA)))], itin> {
  bits<5> rD;
  bits<5> rA;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{15-11} = 0;
}

def ITOFf32 : ConvF<0x04, "lf.itof.s", f32, i32, sint_to_fp, II_FITOFD>;
def FTOIf32 : ConvF<0x05, "lf.ftoi.s", i32, f32, fp_to_sint, II_FITOFS>;

//===----------------------------------------------------------------------===//
// Condition/SF instructions
//===----------------------------------------------------------------------===//

class SetFlagFRR<bits<8> subOp, string asmstr, CondCode Cond, InstrItinClass itin>
  : InstFRR<subOp, (outs), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rA, $rB"),
           [(OR1KSetFlag (f32 GPR:$rA), (f32 GPR:$rB), Cond)], itin> {
  bits<5> rA;
  bits<5> rB;

  let Inst{20-16} = rA;
  let Inst{15-11} = rB;
}

let Defs = [SR_F] in {
  def SFEQrrf32 : SetFlagFRR<0x08, "lf.sfeq.s", SETOEQ, II_SET_FLAG_S>;
  def SFNErrf32 : SetFlagFRR<0x09, "lf.sfne.s", SETUNE, II_SET_FLAG_S>;
  def SFGTrrf32 : SetFlagFRR<0x0a, "lf.sfgt.s", SETOGT, II_SET_FLAG_S>;
  def SFGErrf32 : SetFlagFRR<0x0b, "lf.sfge.s", SETOGE, II_SET_FLAG_S>;
  def SFLTrrf32 : SetFlagFRR<0x0c, "lf.sflt.s", SETOLT, II_SET_FLAG_S>;
  def SFLErrf32 : SetFlagFRR<0x0d, "lf.sfle.s", SETOLE, II_SET_FLAG_S>;
}

let Predicates = [HasCmov] in
  def : Pat<(f32 (OR1KSelect (f32 GPR:$rA),(f32 GPR:$rB))),
            (CMOV GPR:$rA, GPR:$rB)>;

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//===----------------------------------------------------------------------===//

def : Pat<(f32 fpimm:$imm), (ORI (MOVHI (HI16f32 fpimm:$imm)),
                                        (LO16f32 fpimm:$imm))>;
def : Pat<(f32 f32hi16:$imm), (MOVHI (HI16f32 f32hi16:$imm))>;
def : Pat<(f32 f32lo16:$imm), (ORI (f32 R0), (LO16f32 f32lo16:$imm))>;

def : Pat<(f32 (load ADDRri:$src)), (LWZ ADDRri:$src)>;

def : Pat<(store (f32 GPR:$rB), ADDRri:$src), (SW GPR:$rB, ADDRri:$src)>;
