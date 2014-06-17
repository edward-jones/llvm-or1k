//===- OR1K.td - Describe the OR1K Target Machine --------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This is the top level entry point for the OR1K target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// OR1K Subtarget features.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Register File, Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "OR1KRegisterInfo.td"
include "OR1KSchedule.td"
include "OR1KCallingConv.td"
include "OR1KInstrInfo.td"

def OR1KInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// OR1K subtarget features
//===----------------------------------------------------------------------===//
def FeatureMul : SubtargetFeature<"mul", "HasMul", "true",
                                  "Enable hardware multiplier">;
def FeatureMul64 : SubtargetFeature<"mul64", "HasMul64", "true",
                                    "Enable 64-bit hardware multiplier">;
def FeatureDiv : SubtargetFeature<"div", "HasDiv", "true",
                                  "Enable hardware divider">;
def FeatureRor : SubtargetFeature<"ror", "HasRor", "true",
                                  "Enable hardware rotate right">;
def FeatureCmov : SubtargetFeature<"cmov", "HasCmov", "true",
                                   "Enable conditional move">;
def FeatureMAC : SubtargetFeature<"mac", "HasMAC", "true",
                                  "Enable MAC instructions">;
def FeatureExt : SubtargetFeature<"ext", "HasExt", "true",
                                  "Enable sign/zero extend instructions">;
def FeatureSFII : SubtargetFeature<"sfii", "HasSFII", "true",
                                   "Enable set flag if with immediate">;
def FeatureFBit : SubtargetFeature<"fbit", "HasFBit", "true",
                                   "Enable find first/last bit instructions">;

def FeatureNoDelay : SubtargetFeature<"no-delay", "DelaySlotType",
                                      "DelayType::NoDelay",
                                      "Disable delay slots">;
def FeatureCompatDelay : SubtargetFeature<"compat-delay", "DelaySlotType",
                                          "DelayType::CompatDelay",
                                          "Enable compat delay slot">;

def NewABI : SubtargetFeature<"abi-new", "OR1KABI", "NewABI",
                              "Enable the NewABI">;

//===----------------------------------------------------------------------===//
// OR1K processors supported.
//===----------------------------------------------------------------------===//

def OR1KSchedMachineModel : SchedMachineModel {
  let Itineraries = OR1KGenericItineraries;
  let IssueWidth = 1;
  let LoadLatency = 2;
}

def : ProcessorModel<"generic", OR1KSchedMachineModel,
                     [FeatureCompatDelay]>;
def : ProcessorModel<"or1200", OR1KSchedMachineModel,
                     [FeatureMul, FeatureRor, FeatureExt, FeatureSFII]>;
def : ProcessorModel<"pulp", OR1KSchedMachineModel,
                     [FeatureMul, FeatureRor, FeatureExt, FeatureSFII,
                      FeatureCmov, FeatureFBit, NewABI]>;

def OR1KInstPrinter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def OR1KAsmParser : AsmParser {
  let MnemonicContainsDot = 1;
}

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def OR1K : Target {
  // Pull in Instruction Info:
  let InstructionSet = OR1KInstrInfo;
  let AssemblyWriters = [OR1KInstPrinter];
  let AssemblyParsers = [OR1KAsmParser];
}
