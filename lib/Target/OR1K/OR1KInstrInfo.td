//===-- OR1KInstrInfo.td - Target Description for OR1K Target -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the OR1K instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

include "OR1KInstrFormats.td"

//===----------------------------------------------------------------------===//
// Feature support predicates
//===----------------------------------------------------------------------===//

def HasDiv : Predicate<"Subtarget.hasDiv()">,
             AssemblerPredicate<"FeatureDiv">;
def HasMul : Predicate<"Subtarget.hasMul()">,
             AssemblerPredicate<"FeatureMul">;
def HasMul64 : Predicate<"Subtarget.hasMul64()">,
               AssemblerPredicate<"FeatureMul64">;
def HasRor : Predicate<"Subtarget.hasRor()">,
             AssemblerPredicate<"FeatureRor">;
def HasCmov : Predicate<"Subtarget.hasCmov()">,
              AssemblerPredicate<"FeatureCmov">;
def NoCmov : Predicate<"!Subtarget.hasCmov()">,
             AssemblerPredicate<"!FeatureCmov">;
def HasMAC : Predicate<"Subtarget.hasMAC()">,
             AssemblerPredicate<"FeatureMAC">;
def HasExt : Predicate<"Subtarget.hasExt()">,
             AssemblerPredicate<"FeatureExt">;
def HasSFII : Predicate<"Subtarget.hasSFII()">,
              AssemblerPredicate<"FeatureSFII">;
def HasFBit : Predicate<"Subtarget.hasFBit()">,
              AssemblerPredicate<"FeatureFBit">;

//===----------------------------------------------------------------------===//
// Custom SDNodes
//===----------------------------------------------------------------------===//

def callseq_start : SDNode<"ISD::CALLSEQ_START",
                           SDCallSeqStart<[ SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END",
                         SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>,
                         [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def SDT_OR1KCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_OR1KSetFlag : SDTypeProfile<0, 3, [SDTCisSameAs<0, 1>]>;
def SDT_OR1KBrCond : SDTypeProfile<0, 2, [SDTCisVT<0, OtherVT>]>;
def SDT_OR1KSelect : SDTypeProfile<1, 2, [SDTCisSameAs<0, 1>,
                                          SDTCisSameAs<1, 2>]>;

def OR1KCall : SDNode<"OR1KISD::Call", SDT_OR1KCall,
                      [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def OR1KReturn : SDNode<"OR1KISD::Return", SDTNone,
                        [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def OR1KSetFlag : SDNode<"OR1KISD::SetFlag", SDT_OR1KSetFlag, [SDNPOutGlue]>;
def OR1KBrCond : SDNode<"OR1KISD::BrCond", SDT_OR1KBrCond,
                        [SDNPHasChain, SDNPInGlue]>;
def OR1KSelect : SDNode<"OR1KISD::Select", SDT_OR1KSelect, [SDNPInGlue]>;
def OR1KHiLo : SDNode<"OR1KISD::HiLo", SDTIntBinOp>;
def OR1KFF1 : SDNode<"OR1KISD::FF1", SDTIntUnaryOp>;
def OR1KFL1 : SDNode<"OR1KISD::FL1", SDTIntUnaryOp>;

//===----------------------------------------------------------------------===//
// Instruction Operands and Operand Patterns
//===----------------------------------------------------------------------===//

// Extract bits 0-15 (low-end) of an immediate value.
def LO16 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((uint64_t)N->getZExtValue() & 0xffff,
                                   MVT::i32);
}]>;

// Extract bits 16-31 (high-end) of an immediate value.
// Transformation function: shift the immediate value down into the low bits.
def HI16 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((uint64_t)N->getZExtValue() >> 16, MVT::i32);
}]>;

def JumpTargetAsmOperand : AsmOperandClass {
  let Name = "JumpTarget";
  let ParserMethod = "parseJumpTargetOperand";
}

let ParserMatchClass = JumpTargetAsmOperand in {
  def brtarget : Operand<OtherVT>;
  def calltarget : Operand<iPTR>;
}

def s16imm   : Operand<i32> {
  let PrintMethod = "printS16ImmOperand";
}

def immZExt6  : PatLeaf<(imm),
                [{return isInt<6>(N->getZExtValue()); }]>;

def immSExt16 : PatLeaf<(imm),
                [{return isInt<16>(N->getSExtValue()); }]>;

def immZExt16 : PatLeaf<(imm),
                [{return isUInt<16>(N->getZExtValue()); }], LO16>;

def i32lo16 : PatLeaf<(i32 imm), [{
  // i32lo16 predicate - true if the 32-bit immediate has only rightmost 16
  // bits set.
  return ((N->getZExtValue() & 0xFFFFULL) == N->getZExtValue());
}], LO16>;

def i32hi16 : PatLeaf<(i32 imm), [{
  // i32hi16 predicate - true if the 32-bit immediate has only leftmost 16
  // bits set.
  return ((N->getZExtValue() & 0xFFFF0000ULL) == N->getZExtValue());
}], HI16>;

def MemAsmOperand : AsmOperandClass {
  let Name = "Mem";
  let ParserMethod = "parseMemOperand";
}

// Addressing modes.
def ADDRri : ComplexPattern<iPTR, 2, "SelectAddr", [frameindex], []>;

// Address operands.
def MEMri : Operand<iPTR> {
  let PrintMethod = "printMemOperand";
  let EncoderMethod = "getMemoryOpValue";
  let DecoderMethod = "DecodeMemoryValue";
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = MemAsmOperand;
}

def F : PatLeaf<(i32 0)>;
def NF : PatLeaf<(i32 1)>;

//===----------------------------------------------------------------------===//
// Condition/SF instructions
//===----------------------------------------------------------------------===//

class SF_RR<bits<5> op2, string asmstr, CondCode Cond>
  : InstRR<0x9, (outs), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rA, $rB"),
           [(OR1KSetFlag (i32 GPR:$rA), (i32 GPR:$rB), Cond)],
           II_SET_FLAG_IF> {
  bits<5> rA;
  bits<5> rB;

  let Inst{25-21} = op2;
  let Inst{20-16} = rA;
  let Inst{15-11} = rB;
}

class SF_RI<bits<5> op2, string asmstr, CondCode Cond>
  : InstRI<0xf, (outs), (ins GPR:$rA, s16imm:$imm),
           !strconcat(asmstr, "i\t$rA, $imm"),
           [(OR1KSetFlag (i32 GPR:$rA), immSExt16:$imm, Cond)],
           II_SET_FLAG_IF> {
  bits<5> rA;
  bits<16> imm;

  let Inst{25-21} = op2;
  let Inst{20-16} = rA;
  let Inst{15-0} = imm;
}

multiclass SF<bits<5> op2Val, string asmstr, CondCode Cond> {
  def _rr : SF_RR<op2Val, asmstr, Cond>;
  let Predicates=[HasSFII] in
    def _ri : SF_RI<op2Val, asmstr, Cond>;
}

// l.sfxx instructions.
let Defs = [SR_F] in {
  defm SFEQ  : SF<0x0, "l.sfeq",  SETEQ>;
  defm SFNE  : SF<0x1, "l.sfne",  SETNE>;
  defm SFGTU : SF<0x2, "l.sfgtu", SETUGT>;
  defm SFGEU : SF<0x3, "l.sfgeu", SETUGE>;
  defm SFLTU : SF<0x4, "l.sfltu", SETULT>;
  defm SFLEU : SF<0x5, "l.sfleu", SETULE>;
  defm SFGTS : SF<0xa, "l.sfgts", SETGT>;
  defm SFGES : SF<0xb, "l.sfges", SETGE>;
  defm SFLTS : SF<0xc, "l.sflts", SETLT>;
  defm SFLES : SF<0xd, "l.sfles", SETLE>;
}

//===----------------------------------------------------------------------===//
// ALU instructions
//===----------------------------------------------------------------------===//

class ALU_RI<bits<4> subOp, dag outs, dag ins, string asmstr,
             list<dag> pattern, InstrItinClass itin>
  : InstRI<subOp, outs, ins, asmstr, pattern, itin> {
  bits<5> rD;
  bits<5> rA;
  bits<16> imm16;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{15-0} = imm16;
}

class ALU_RIs<bits<4> subOp, string asmstr, SDNode OpNode, InstrItinClass itin>
  : ALU_RI<subOp, (outs GPR:$rD), (ins GPR:$rA, s16imm:$imm16),
           !strconcat(asmstr, "\t$rD, $rA, $imm16"),
           [(set GPR:$rD, (OpNode GPR:$rA, immSExt16:$imm16))], itin>;

class ALU_RIz<bits<4> subOp, string asmstr, SDNode OpNode, InstrItinClass itin>
  : ALU_RI<subOp, (outs GPR:$rD), (ins GPR:$rA, i32imm:$imm16),
           !strconcat(asmstr, "\t$rD, $rA, $imm16"),
           [(set GPR:$rD, (OpNode GPR:$rA, immZExt16:$imm16))], itin>;

let isAsCheapAsAMove = 1 in {
  let Defs = [SR_CY] in {
    def ADDI: ALU_RIs<0x7, "l.addi", add, II_ALU>;
    let Uses = [SR_CY] in def ADDIC: ALU_RIs<0x8, "l.addic", adde, II_ALU>;
  }
  def ANDI  : ALU_RIz<0x9, "l.andi", and, II_ALU>;
  def ORI   : ALU_RIz<0xa, "l.ori", or, II_ALU>;
  def XORI  : ALU_RIs<0xb, "l.xori", xor, II_ALU>;
}

def : Pat<(addc GPR:$rA, immSExt16:$imm16),
          (ADDI GPR:$rA, immSExt16:$imm16)>;

let Predicates=[HasMul], Defs = [SR_OV] in
  def MULI  : ALU_RIs<0xc, "l.muli", mul, II_MUL>;

class ALU_RR<bits<2> op2, bits<4> op3, string asmstr,
             list<dag> pattern, InstrItinClass itin>
  : InstRR<0x8, (outs GPR:$rD), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rD, $rA, $rB"), pattern, itin> {
  bits<5> rD;
  bits<5> rA;
  bits<5> rB;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{15-11} = rB;

  let Inst{9-8} = op2;
  let Inst{3-0} = op3;
}

class ALU_STD_RR<bits<4> op, string asmstr, SDNode OpNode, InstrItinClass itin>
  : ALU_RR<0x0, op, asmstr,
           [(set GPR:$rD, (OpNode (i32 GPR:$rA), (i32 GPR:$rB)))], itin>;

let isAsCheapAsAMove = 1 in {
  let isCommutable=1, Defs = [SR_CY] in {
    def ADD  : ALU_STD_RR<0x0, "l.add", add, II_ALU>;
    let Uses = [SR_CY] in def ADDC : ALU_STD_RR<0x1, "l.addc", adde, II_ALU>;
  }

  def SUB  : ALU_STD_RR<0x2, "l.sub", sub, II_ALU>;

  let isCommutable=1 in {
    def AND  : ALU_STD_RR<0x3, "l.and", and, II_ALU>;
    def OR   : ALU_STD_RR<0x4, "l.or", or, II_ALU>;
    def XOR  : ALU_STD_RR<0x5, "l.xor", xor, II_ALU>;
 }
}

def : Pat<(addc GPR:$rA, GPR:$rB),
          (ADD GPR:$rA, GPR:$rB)>;

let Uses = [SR_F], Predicates = [HasCmov] in
  def CMOV : ALU_STD_RR<0xe, "l.cmov", OR1KSelect, II_CMOV>;

class ALU_OPT_RR<bits<4> subOp, string asmstr, SDNode OpNode,
                 InstrItinClass itin>
  : ALU_RR<0x3, subOp, asmstr,
           [(set GPR:$rD, (OpNode (i32 GPR:$rA), (i32 GPR:$rB)))], itin>;

let isCommutable = 1, Predicates = [HasMul] in {
  let Defs = [SR_OV] in def MUL  : ALU_OPT_RR<0x6, "l.mul", mul, II_MUL>;
  let Defs = [SR_CY] in def MULU : ALU_OPT_RR<0xb, "l.mulu", mul, II_MUL>;
}
let Predicates = [HasDiv] in {
  let Defs = [SR_OV] in def DIV  : ALU_OPT_RR<0x9, "l.div", sdiv, II_DIV>;
  let Defs = [SR_CY] in def DIVU : ALU_OPT_RR<0xa, "l.divu", udiv, II_DIV>;
}

class ALU_D<bits<2> op2, bits<4> op3, string asmstr,
            list<dag> pattern, InstrItinClass itin>
  : InstRR<0x8, (outs), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rA, $rB"), pattern, itin> {
  bits<5> rA;
  bits<5> rB;

  let Inst{20-16} = rA;
  let Inst{15-11} = rB;

  let Inst{9-8} = op2;
  let Inst{3-0} = op3;
}

class ALU_MULD<bits<4> subOp, string asmstr, InstrItinClass itin>
  : ALU_D<0x3, subOp, asmstr, [], itin>;

let Predicates=[HasMul64] in {
  let isCommutable = 1, hasSideEffects = 0, Defs = [MACLO, MACHI] in {
    def MULD  : ALU_MULD<0x7, "l.muld", II_MUL>;
    def MULDU : ALU_MULD<0xc, "l.muldu", II_MUL>;
  }
}

class ALU2_RR<bits<2> op1, bits<4> op2, string asmstr, SDNode OpNode>
  : InstRR<0x8, (outs GPR:$rD), (ins GPR:$rA),
           !strconcat(asmstr, "\t$rD, $rA"),
           [(set GPR:$rD, (OpNode GPR:$rA))], II_ALU> {
  bits<5> rD;
  bits<5> rA;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;

  let Inst{9-8} = op1;
  let Inst{3-0} = op2;
}

let Predicates = [HasFBit] in {
def FF1 : ALU2_RR<0x0, 0xf, "l.ff1", OR1KFF1>;
def FL1 : ALU2_RR<0x1, 0xf, "l.fl1", OR1KFL1>;
}

//===----------------------------------------------------------------------===//
// MOVHI instruction
//===----------------------------------------------------------------------===//

class MOVHI_I <dag outs, dag ins, list<dag> pattern, InstrItinClass itin>
  : InstOR1K<outs, ins,  "l.movhi\t$rD, $imm", pattern, itin> {
  bits<16> imm;
  bits<5> rD;
  let optype = 0;
  let opcode = 0x6;
  let Inst{16} = 0;
  let Inst{15-0} = imm;
  let Inst{25-21} = rD;
}

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
  def MOVHI : MOVHI_I<(outs GPR:$rD), (ins i32imm:$imm),
                      [(set GPR:$rD, i32hi16:$imm)], II_MOVHI>;
}

//===----------------------------------------------------------------------===//
// SHIFT instructions
//===----------------------------------------------------------------------===//

class SHIFT_RR<bits<2> op, string asmstr, SDNode OpNode, InstrItinClass itin>
  : InstRR<0x8, (outs GPR:$rD), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rD, $rA, $rB"),
           [(set GPR:$rD, (OpNode GPR:$rA, GPR:$rB))], itin> {
  bits<5> rD;
  bits<5> rA;
  bits<5> rB;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{15-11} = rB;

  let Inst{9-8} = 0x0;
  let Inst{7-6} = op;
  let Inst{3-0} = 0x8;
}

class SHIFT_RI<bits<2> op, string asmstr, SDNode OpNode, InstrItinClass itin>
  : InstRI<0xE, (outs GPR:$rD), (ins GPR:$rA, i32imm:$imm),
           !strconcat(asmstr, "i\t$rD, $rA, $imm"),
           [(set GPR:$rD, (OpNode GPR:$rA, immZExt6:$imm))], itin> {
  bits<5> rD;
  bits<5> rA;
  bits<6> imm;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{7-6} = op;
  let Inst{5-0} = imm;
}

multiclass SHIFT<bits<2> op, string asmstr,
                 SDNode OpNode, InstrItinClass itin> {
  def _rr : SHIFT_RR<op, asmstr, OpNode, itin>;
  def _ri : SHIFT_RI<op, asmstr, OpNode, itin>;
}


defm SLL : SHIFT<0x0, "l.sll", shl, II_ALU>;
defm SRL : SHIFT<0x1, "l.srl", srl, II_ALU>;
defm SRA : SHIFT<0x2, "l.sra", sra, II_ALU>;
let Predicates=[HasRor] in
  defm ROR : SHIFT<0x3, "l.ror", rotr, II_ROT>;

//===----------------------------------------------------------------------===//
// STORE instructions
//===----------------------------------------------------------------------===//

class STORE<bits<4> subOp, string asmstring, list<dag> pattern>
  : InstRR<subOp, (outs), (ins GPR:$rB, MEMri:$dst),
          !strconcat(asmstring, "\t$dst, $rB"), pattern, II_STORE> {
  bits<21> dst;
  bits<5> rB;

  let Inst{25-21} = dst{15-11};
  let Inst{20-16} = dst{20-16};
  let Inst{15-11} = rB;
  let Inst{10-0} = dst{10-0};
}

class STOREi32<bits<4> subOp, string asmstring, PatFrag opNode>
  : STORE<subOp, asmstring, [(opNode (i32 GPR:$rB), ADDRri:$dst)]>;

//def SD : STORE<0x4, "l.sd">;
def SW : STOREi32<0x5, "l.sw", store>;
def SB : STOREi32<0x6, "l.sb", truncstorei8>;
def SH : STOREi32<0x7, "l.sh", truncstorei16>;

//===----------------------------------------------------------------------===//
// LOAD instructions
//===----------------------------------------------------------------------===//

class LOAD<bits<4> subop, string asmstring, list<dag> pattern>
  : InstRI<subop, (outs GPR:$rD), (ins MEMri:$src),
           !strconcat(asmstring, "\t$rD, $src"), pattern, II_LOAD> {
  bits<5> rD;
  bits<21> src;

  let Inst{25-21} = rD;
  let Inst{20-0} = src;
}

class LOADi32<bits<4> subop, string asmstring, PatFrag opNode>
  : LOAD<subop, asmstring, [(set (i32 GPR:$rD), (opNode ADDRri:$src))]>;

//def LD : LOAD<0x0, "l.ld">;
def LWZ : LOADi32<0x1, "l.lwz", load>;
def LWS : LOAD<0x2, "l.lws", []>;
def LBZ : LOADi32<0x3, "l.lbz", zextloadi8>;
def LBS : LOADi32<0x4, "l.lbs", sextloadi8>;
def LHZ : LOADi32<0x5, "l.lhz", zextloadi16>;
def LHS : LOADi32<0x6, "l.lhs", sextloadi16>;

//===----------------------------------------------------------------------===//
// BRANCH instructions
//===----------------------------------------------------------------------===//

class BRANCH<bits<4> op, string asmstring,
             list<dag> pattern, InstrItinClass itin>
  : InstBI<op, (outs), (ins brtarget:$dst),
           !strconcat(asmstring, "\t$dst"), pattern, itin> {
  bits<28> dst;

  let Inst{25-0} = dst{27-2};
}

class BRANCHL<bits<4> op, string asmstring,  InstrItinClass itin>
  : InstBI<op, (outs), (ins calltarget:$dst),
           !strconcat(asmstring, "\t$dst"), [], itin> {
  bits<28> dst;

  let Inst{25-0} = dst{27-2};
}

class BRANCH_R<bits<4> op, string asmstring,
               list<dag> pattern, InstrItinClass itin>
  : InstBR<op, (outs), (ins GPR:$rB),
           !strconcat(asmstring, "\t$rB"), pattern, itin> {
  bits<5> rB;

  let Inst{15-11} = rB;
}

class BRANCHL_R<bits<4> op, string asmstring,  InstrItinClass itin>
  : InstBR<op, (outs), (ins GPR:$rB),
           !strconcat(asmstring, "\t$rB"), [(OR1KCall GPR:$rB)], itin> {
  bits<5> rB;

  let Inst{15-11} = rB;
}

// Jump/Branch.
let isBranch = 1, isTerminator = 1, hasDelaySlot = 1 in {
  let isBarrier = 1 in {
    def J : BRANCH<0x0, "l.j", [(br bb:$dst)], II_JUMP>;
    let isIndirectBranch = 1 in {
      def JR : BRANCH_R<0x1, "l.jr", [(brind GPR:$rB)], II_JUMP>;
    }
  }
  let Uses = [SR_F] in {
    def BNF : BRANCH<0x3, "l.bnf", [(OR1KBrCond bb:$dst, NF)], II_BRANCH>;
    def BF  : BRANCH<0x4, "l.bf",  [(OR1KBrCond bb:$dst, F)], II_BRANCH>;
  }
}

// Jump and link.
let isCall = 1, hasDelaySlot = 1, Uses = [R1],
    // Potentially clobbered registers.
    Defs = [R3, R4, R5, R6, R7, R8, R9, R11, R12, R13, R15, R17, R19, R21, R23,
            R25, R27, R29, R31] in {
  def JAL  : BRANCHL<0x1, "l.jal", II_JUMP>;
  def JALR : BRANCHL_R<0x2, "l.jalr", II_JUMP>;
}

class NOP_I<bits<2> op, string asmstr, InstrItinClass itin>
  : InstBI<0x5, (outs), (ins i16imm:$imm),
           !strconcat(asmstr, "\t$imm"), [], itin> {
  bits<16> imm;

  let Inst{25-24} = op;
  let Inst{15-0} = imm;
}

let hasSideEffects = 0 in
  def NOP : NOP_I<0x1, "l.nop", II_NOP>;

let isReturn = 1, isTerminator = 1, hasDelaySlot = 1,
    isBarrier = 1, Uses = [R9] in {
  def RET : FixedOp<0x44004800, "l.jr\tr9", [(OR1KReturn)]>;
}

//===----------------------------------------------------------------------===//
// EXTEND with sign/zero instructions
//===----------------------------------------------------------------------===//

class EXTEND<bits<4> op1, bits<4> op2, string asmstring, list<dag> pattern>
  : InstRR<0x8, (outs GPR:$rD), (ins GPR:$rA),
           !strconcat(asmstring, "\t$rD, $rA"), pattern, II_SIGNZERO_EXT> {
  bits<5> rD;
  bits<5> rA;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;

  let Inst{9-6} = op1;
  let Inst{3-0} = op2;
}

let Predicates=[HasExt] in {
  def EXT_BYTE_SIGN :
    EXTEND<0x1, 0xc, "l.extbs", [(set GPR:$rD, (sext_inreg GPR:$rA, i8))]>;
  def EXT_HALF_SIGN :
    EXTEND<0x0, 0xc, "l.exths", [(set GPR:$rD, (sext_inreg GPR:$rA, i16))]>;
  def EXT_WORD_SIGN : EXTEND<0x0, 0xd, "l.extws", []>;

  def EXT_BYTE_ZERO : EXTEND<0x3, 0xc, "l.extbz", []>;
  def EXT_HALF_ZERO : EXTEND<0x2, 0xc, "l.exthz", []>;
  def EXT_WORD_ZERO : EXTEND<0x1, 0xd, "l.extwz", []>;

  def : Pat<(and GPR:$rA, 255), (EXT_BYTE_ZERO GPR:$rA)>;
  def : Pat<(and GPR:$rA, 65535), (EXT_HALF_ZERO GPR:$rA)>;
}

//===----------------------------------------------------------------------===//
// Synchronization instructions
//===----------------------------------------------------------------------===//

let hasSideEffects = 1 in {
  def CONTX_SYNC : FixedOp<0x23000000, "l.csync", [(int_or1k_csync)]>;
  def MEM_SYNC : FixedOp<0x22000000, "l.msync", [(int_or1k_msync)]>;
  def PIPE_SYNC : FixedOp<0x22800000, "l.psync", [(int_or1k_psync)]>;
}

//===----------------------------------------------------------------------===//
// OS interaction instructions
//===----------------------------------------------------------------------===//

class InstOS<bits<16> op, string asmstring, list<dag> pattern>
  : InstOR1K<(outs), (ins i16imm:$imm),
	  !strconcat(asmstring, "\t$imm"), pattern> {
  bits<16> imm;

  let Inst{31-16} = op;
  let Inst{15-0} = imm;
}

let hasSideEffects = 1 in {
  def SYS : InstOS<0x2000, "l.sys", [(int_or1k_sys immZExt16:$imm)]>;
  def TRAP : InstOS<0x2100, "l.trap", [(int_or1k_trap immZExt16:$imm)]>;
  def RFE : FixedOp<0x24000000, "l.rfe", [(int_or1k_rfe)]>;
}

//===----------------------------------------------------------------------===//
// MAC instructions
//===----------------------------------------------------------------------===//

class MAC_RR<bits<4> op, string asmstr, list<dag> pattern>
  : InstRR<0x1, (outs), (ins GPR:$rA, GPR:$rB),
           !strconcat(asmstr, "\t$rA, $rB"), pattern> {
  bits<5> rA;
  bits<5> rB;

  let Inst{20-16} = rA;
  let Inst{15-11} = rB;

  let Inst{3-0} = op;
}

class MAC_RI<string asmstr, list<dag> pattern>
  : InstBR<0x3, (outs), (ins GPR:$rA, s16imm:$imm),
           !strconcat(asmstr, "\t$rA, $imm"), pattern> {
  bits<5> rA;
  bits<16> imm;

  let Inst{20-16} = rA;
  let Inst{15-0} = imm;
}

class MAC_R<string asmstr, list<dag> pattern>
  : InstBI<0x6, (outs GPR:$rD), (ins),  !strconcat(asmstr, "\t$rD"), pattern> {
  bits<5> rD;

  let Inst{25-21} = rD;
  let Inst{16-0} = 0x10000;
}

let Predicates=[HasMAC] in {
  def MAC : MAC_RR<0x1, "l.mac", []>;
  def MSB : MAC_RR<0x2, "l.msb", []>;

  def MACI : MAC_RI<"l.maci", []>;

  def MACU : MAC_RR<0x3, "l.macu", []>;
  def MSBU : MAC_RR<0x4, "l.msbu", []>;

  def MACRC : MAC_R<"l.macrc", []>;
}

//===----------------------------------------------------------------------===//
// MOVE from/to special register instructions
//===----------------------------------------------------------------------===//

class MOVE_FROM_SP<string asmstr, list<dag> pattern>
  : InstRI<0xd, (outs GPR:$rD), (ins GPR:$rA, s16imm:$imm),
    !strconcat(asmstr, "\t$rD, $rA, $imm"), pattern> {
  bits<5> rD;
  bits<5> rA;
  bits<16> imm;

  let Inst{25-21} = rD;
  let Inst{20-16} = rA;
  let Inst{15-0} = imm;
}

class MOVE_TO_SP<string asmstr, list<dag> pattern>
  : InstRR<0x0, (outs), (ins GPR:$rA, GPR:$rB, s16imm:$imm),
    !strconcat(asmstr, "\t$rA, $rB, $imm"), pattern> {
  bits<5> rA;
  bits<5> rB;
  bits<16> imm;

  let Inst{25-21} = imm{15-11};
  let Inst{20-16} = rA;
  let Inst{15-11} = rB;
  let Inst{10-0} = imm{10-0};
}

let hasSideEffects = 1 in {
  def MTSPR : MOVE_TO_SP<"l.mtspr", []>;
  def MFSPR : MOVE_FROM_SP<"l.mfspr", []>;
}

//===----------------------------------------------------------------------===//
// Pseudo Instructions
//===----------------------------------------------------------------------===//

// ADJCALLSTACKDOWN/UP implicitly use/def R1 because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
// Pessimistically assume ADJCALLSTACKDOWN / ADJCALLSTACKUP will become
// sub / add which can clobber R1.
let Defs = [R1], Uses = [R1] in {
  def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt),
                                "#ADJCALLSTACKDOWN $amt",
                                [(callseq_start timm:$amt)]>;
  def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "#ADJCALLSTACKUP $amt1 $amt2",
                                [(callseq_end timm:$amt1, timm:$amt2)]>;
}

let Uses = [SR_F], usesCustomInserter = 1, Predicates = [NoCmov] in {
  def SELECT : Pseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$src2),
                      "#SELECT $dst, $src, $src2", []>;
}

let Predicates = [NoCmov] in {
  def : Pat<(i32 (OR1KSelect (i32 GPR:$rA), (i32 GPR:$rB))),
            (SELECT GPR:$rA, GPR:$rB)>;

  def : Pat<(f32 (OR1KSelect (f32 GPR:$rA), (f32 GPR:$rB))),
            (SELECT GPR:$rA, GPR:$rB)>;
}

let hasSideEffects = 0, neverHasSideEffects = 1, Defs = [R9], Uses = [R9] in
  def GET_GLOBAL_BASE : Pseudo<(outs GPR:$gp), (ins),
                               "#GLOBAL_BASE $gp", []>;

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//===----------------------------------------------------------------------===//

// signed 16-bit immediate.
def : Pat<(i32 immSExt16:$imm), (ADDI (i32 R0), imm:$imm)>;

// unsigned 16-bit immediate.
def : Pat<(i32 immZExt16:$imm), (ORI (i32 R0), imm:$imm)>;

// arbitrary immediate.
def : Pat<(i32 imm:$imm), (ORI (MOVHI (HI16 imm:$imm)), (LO16 imm:$imm))>;

// Calls.
def : Pat<(OR1KCall tglobaladdr:$dst), (JAL tglobaladdr:$dst)>;
def : Pat<(OR1KCall texternalsym:$dst), (JAL texternalsym:$dst)>;

// Loads.
def : Pat<(extloadi8  ADDRri:$src), (i32 (LBZ ADDRri:$src))>;
def : Pat<(extloadi16 ADDRri:$src), (i32 (LHZ ADDRri:$src))>;

// GlobalAddress, ExternalSymbol, BlockAddress and Jumptable.
def : Pat<(OR1KHiLo tglobaladdr:$dst_hi, tglobaladdr:$dst_lo),
          (ORI (MOVHI tglobaladdr:$dst_hi), tglobaladdr:$dst_lo)>;
def : Pat<(OR1KHiLo texternalsym:$dst_hi, texternalsym:$dst_lo),
          (ORI (MOVHI texternalsym:$dst_hi), texternalsym:$dst_lo)>;
def : Pat<(OR1KHiLo tblockaddress:$dst_hi, tblockaddress:$dst_lo),
          (ORI (MOVHI tblockaddress:$dst_hi), tblockaddress:$dst_lo)>;
def : Pat<(OR1KHiLo tjumptable:$dst_hi, tjumptable:$dst_lo),
          (ORI (MOVHI tjumptable:$dst_hi), tjumptable:$dst_lo)>;

// We have R0 always set to zero!
def : Pat<(i32 0), (i32 (COPY (i32 R0)))>;

//===----------------------------------------------------------------------===//
// Assembler Alias
//===----------------------------------------------------------------------===//

def : InstAlias<"l.nop", (NOP 0), 1>;

//===----------------------------------------------------------------------===//
// Floating Point Support
//===----------------------------------------------------------------------===//

include "OR1KInstrFPU.td"
