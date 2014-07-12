//===-- OR1KSelectionDAGInfo.cpp - OR1K SelectionDAG Info -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the OR1KSelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#include "OR1KTargetMachine.h"

#define DEBUG_TYPE "or1k-selectiondag-info"

using namespace llvm;

OR1KSelectionDAGInfo::OR1KSelectionDAGInfo(const DataLayout *DL)
    : TargetSelectionDAGInfo(DL) {}

OR1KSelectionDAGInfo::~OR1KSelectionDAGInfo() {}
