//===-- OR1KSchedule.td - Mips Scheduling Definitions ------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Functional units.
//===----------------------------------------------------------------------===//
def FU_ALU : FuncUnit;
def FU_FPU : FuncUnit;

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for OR1K
//===----------------------------------------------------------------------===//
// standard ALU instructions (ADD, ADDC, ADDI, ADDIC, AND, ANDI, FF, FL, OR,
// ORI, SLL, SLLI, SRA, SRAI, SRL, SRLI, XOR, XORI, SUB)
def II_ALU : InstrItinClass;

// branch if/if not flag instructions (BF, BNF)
def II_BRANCH : InstrItinClass;

// conditional move instruction (CMOV)
def II_CMOV : InstrItinClass;

// division instructions (DIV, DIVU)
def II_DIV : InstrItinClass;

// zero/sign extension instructions (EXTBS, EXTHS, EXTWS, EXTBZ, EXTHZ, EXTWZ)
def II_SIGNZERO_EXT : InstrItinClass;

// jump instructions (J, JAL, JALR, JR)
def II_JUMP : InstrItinClass;

// move immediate high (MOVHI)
def II_MOVHI : InstrItinClass;

// load instuctions (LBS, LBZ, LD, LHS, LHZ, LWS, LWZ)
def II_LOAD : InstrItinClass;

// move from/to special register (MFSPR, MTSPR)
def II_MOVE_SPR : InstrItinClass;

// multiplication instructions (MUL, MULI, MULU)
def II_MUL : InstrItinClass;

// nop instruction (NOP)
def II_NOP : InstrItinClass;

// rotate instructions (ROR, RORI)
def II_ROT : InstrItinClass;

// set flag instructions (SFEQ, SFEQI, SFGES, SFGESI, SFGEU, SFGEUI, SFGTS,
// SFGTSI, SFGTU, SFGTUI, SFLES, SFLESI, SFLEU, SFLEUI, SFLTS, SFLTSI, SFLTU,
// SFLTUI, SFNE, SFNEI)
def II_SET_FLAG_IF : InstrItinClass;

// store instructions (SB, SH, SW, SD)
def II_STORE : InstrItinClass;

def II_MAC : InstrItinClass;
def II_MACI : InstrItinClass;
def II_MSB : InstrItinClass;
def II_MACRC : InstrItinClass;

def II_CSYNC : InstrItinClass;
def II_MSYNC : InstrItinClass;
def II_PSYNC : InstrItinClass;

def II_SYS : InstrItinClass;
def II_TRAP : InstrItinClass;
def II_RFE : InstrItinClass;

def II_FADDD : InstrItinClass;
def II_FCUST1D : InstrItinClass;
def II_FDIVD : InstrItinClass;
def II_FFTOID : InstrItinClass;
def II_FITOFD : InstrItinClass;
def II_FMADDD : InstrItinClass;
def II_FMULD : InstrItinClass;
def II_FREMD : InstrItinClass;
def II_FSUBD : InstrItinClass;

def II_FADDS : InstrItinClass;
def II_FCUST1S : InstrItinClass;
def II_FDIVS : InstrItinClass;
def II_FFTOIS : InstrItinClass;
def II_FITOFS : InstrItinClass;
def II_FMADDS : InstrItinClass;
def II_FMULS : InstrItinClass;
def II_FREMS : InstrItinClass;
def II_FSUBS : InstrItinClass;

// set flag if (single precision) instruction (FSFEQS, FSFGES, FSFGTS, FSFLES
// FSFLTS, FSFNES)
def II_SET_FLAG_S : InstrItinClass;

// set flag if (double precision) instructions (FSFEQD, FSFGED, FSFGTD, FSFLED,
// FSFLTD, FSFNED)
def II_SET_FLAG_D : InstrItinClass;

//===----------------------------------------------------------------------===//
// OR1K Generic instruction itineraries.
//===----------------------------------------------------------------------===//
def OR1KGenericItineraries : ProcessorItineraries<[FU_ALU, FU_FPU], [], [
  InstrItinData< II_ALU             , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_BRANCH          , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_CMOV            , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_DIV             , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_SIGNZERO_EXT    , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_JUMP            , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_MOVHI           , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_LOAD            , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_MOVE_SPR        , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_MUL             , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_NOP             , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_ROT             , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_SET_FLAG_IF     , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_STORE           , [InstrStage<1,  [FU_ALU]>]>,
  InstrItinData< II_FADDS           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FCUST1S         , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FDIVS           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FFTOIS          , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FITOFS          , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FMADDS          , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FMULS           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FREMS           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FSUBS           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_SET_FLAG_S      , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FADDD           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FCUST1D         , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FDIVD           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FFTOID          , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FITOFD          , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FMADDD          , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FMULD           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FREMD           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_FSUBD           , [InstrStage<1,  [FU_FPU]>]>,
  InstrItinData< II_SET_FLAG_D      , [InstrStage<1,  [FU_FPU]>]>
]>;
