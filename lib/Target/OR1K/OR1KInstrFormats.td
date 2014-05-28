//===- OR1KInstrFormats.td - OR1K Instruction Formats ------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

class InstOR1K<dag outs, dag ins, string asmstr,
              list<dag> pattern, InstrItinClass itin = NoItinerary>
  : Instruction {
  field bits<32> Inst;
  field bits<32> SoftFail = 0;
  let Size = 4;

  let Namespace = "OR1K";
  let DecoderNamespace = "OR1K";

  let Itinerary = itin;

  bits<2> optype;
  bits<4> opcode;
  let Inst{31-30} = optype;
  let Inst{29-26} = opcode;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}


class InstRR<bits<4> op, dag outs, dag ins, string asmstr,
             list<dag> pattern, InstrItinClass itin = NoItinerary>
  : InstOR1K<outs, ins, asmstr, pattern, itin> {
  let optype = 0b11;
  let opcode = op;
}

class InstRI<bits<4> op, dag outs, dag ins, string asmstr,
             list<dag> pattern, InstrItinClass itin = NoItinerary>
  : InstOR1K<outs, ins, asmstr, pattern, itin> {
  let optype = 0b10;
  let opcode = op;
}

class InstBR<bits<4> op, dag outs, dag ins, string asmstr,
             list<dag> pattern, InstrItinClass itin = NoItinerary>
  : InstOR1K<outs, ins, asmstr, pattern, itin> {
  let optype = 0b1;
  let opcode = op;
}

class InstBI<bits<4> op, dag outs, dag ins, string asmstr,
             list<dag>pattern, InstrItinClass itin = NoItinerary>
  : InstOR1K<outs, ins, asmstr, pattern, itin> {
  let optype = 0;
  let opcode = op;
}

// Floating point instructions.
class InstFRR<bits<8> op, dag outs, dag ins, string asmstr,
              list<dag> pattern, InstrItinClass itin = NoItinerary>
  : InstOR1K<outs, ins, asmstr, pattern, itin> {
  let optype = 0b11;
  let opcode = 0b0010;
  let Inst{7-0} = op;
}

// Fixed opcode instructions.
class FixedOp<bits<32> op, string asmstr, list<dag> pattern>
  : InstOR1K<(outs), (ins), asmstr, pattern> {
  let Inst{31-0} = op;
}

// Pseudo instructions.
class Pseudo<dag outs, dag ins, string asmstr,
             list<dag> pattern, InstrItinClass itin = NoItinerary>
  : InstOR1K<outs, ins, asmstr, pattern, itin> {
  let Inst{15-0} = 0;
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}
