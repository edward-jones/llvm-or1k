//===-- OR1KTargetInfo.cpp - OR1K Target Implementation -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "OR1K.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheOR1KbeTarget;
Target llvm::TheOR1KleTarget;

extern "C" void LLVMInitializeOR1KTargetInfo() { 
  RegisterTarget<Triple::or1k> X(TheOR1KbeTarget, "or1k", "OR1K");
  RegisterTarget<Triple::or1kle> Y(TheOR1KleTarget, "or1kle", "OR1KLE");
}
