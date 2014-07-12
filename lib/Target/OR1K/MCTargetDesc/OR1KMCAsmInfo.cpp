//===-- OR1KMCAsmInfo.cpp - OR1K asm properties -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the OR1KMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "OR1KMCAsmInfo.h"
#include "llvm/ADT/Triple.h"

#define DEBUG_TYPE "or1k-mcasm-info"

using namespace llvm;

OR1KMCAsmInfo::OR1KMCAsmInfo(StringRef TT) {
  if (Triple(TT).getArch() == Triple::or1k)
    IsLittleEndian = false;

  PrivateGlobalPrefix = ".L";
  WeakRefDirective = "\t.weak\t";

  // OR1K assembly requires ".section" before ".bss"
  UsesELFSectionDirectiveForBSS = true;

  // Enable debug information
  SupportsDebugInformation = true;
  DwarfRegNumForCFI = true;
  MinInstAlignment = 4;
}
