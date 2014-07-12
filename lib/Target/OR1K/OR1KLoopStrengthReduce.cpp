//===-- OR1KLoopStrengthReduce.cpp - Custom LSR -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a simple strength reduction transformation to expose
// pointer increments within loops.
//
//===----------------------------------------------------------------------===//

#include "OR1K.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Analysis/IVUsers.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include <algorithm>

#define DEBUG_TYPE "or1k-loop-strength-reduce"

using namespace llvm;

namespace {
class OR1KLoopStrengthReduction : public LoopPass {
  IVUsers *IU;
  ScalarEvolution *SE;
  DominatorTree *DT;
public:
  static char ID;

  OR1KLoopStrengthReduction() : LoopPass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
    INITIALIZE_PASS_DEPENDENCY(ScalarEvolution)
    INITIALIZE_PASS_DEPENDENCY(IVUsers)
    INITIALIZE_PASS_DEPENDENCY(LoopInfo)
    INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
    INITIALIZE_PASS_DEPENDENCY(LCSSA)
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    AU.addPreserved<LoopInfo>();
    AU.addRequiredID(LoopSimplifyID);
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
    AU.addRequired<ScalarEvolution>();
    AU.addPreserved<ScalarEvolution>();
    AU.addRequiredID(LoopSimplifyID);
    AU.addPreservedID(LoopSimplifyID);
    AU.addRequired<IVUsers>();
    AU.setPreservesCFG();
  }

  const char *getPassName() const override {
    return "OR1K Loop Strength Reduction";
  }

  bool runOnLoop(Loop *L, LPPassManager &LPM);
};

char OR1KLoopStrengthReduction::ID = 0;
}

bool OR1KLoopStrengthReduction::runOnLoop(Loop *L, LPPassManager &LPM) {
  IU = &getAnalysis<IVUsers>();
  SE = &getAnalysis<ScalarEvolution>();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  
  for (IVUsers::iterator UI = IU->begin(), UE = IU->end(); UI != UE; ++UI) {
    IVStrideUse &IVS = *UI;
    Value *V = IVS.getOperandValToReplace();
    
    if (!V || !isa<GetElementPtrInst>(V)) continue;

    GetElementPtrInst *GEP = cast<GetElementPtrInst>(V);
    Value *Src = GEP->getPointerOperand();
    if (isa<PHINode>(Src) && !L->isLoopInvariant(Src)) continue;

    SCEVExpander R(*SE, "sr");
    R.enableLSRMode();
    R.disableCanonicalMode();
    R.setPostInc(IVS.getPostIncLoops());

    const SCEV *S = IU->getExpr(IVS);

    PostIncLoopSet &Loops = const_cast<PostIncLoopSet &>(IVS.getPostIncLoops());
    S = TransformForPostIncUse(Denormalize, S,
                               IVS.getUser(), GEP,
                               Loops, *SE, *DT);
    Value *NV = R.expandCodeFor(S, GEP->getType(), GEP);

    GEP->replaceAllUsesWith(NV);
    
    RecursivelyDeleteTriviallyDeadInstructions(GEP);
  }

  return true;
}

Pass *llvm::createOR1KLoopStrengthReduction() {
  return new OR1KLoopStrengthReduction();
}
