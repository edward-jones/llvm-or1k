//===- IntrinsicsOR1K.td - Defines OR1K intrinsics ---------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines all the OR1K-specific intrinsics.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "or1k" in {  // All intrinsics start with "llvm.or1k.".

//===----------------------------------------------------------------------===//
// OR1K GCC Builtin and Intrinsic for OS interaction and hardware
// synchronization instruction.
//===----------------------------------------------------------------------===//

def int_or1k_csync : GCCBuiltin<"__builtin_or1k_csync">,
  Intrinsic<[], [], []>;

def int_or1k_msync : GCCBuiltin<"__builtin_or1k_msync">,
  Intrinsic<[], [], []>;

def int_or1k_psync : GCCBuiltin<"__builtin_or1k_psync">,
  Intrinsic<[], [], []>;

def int_or1k_sys : GCCBuiltin<"__builtin_or1k_sys">,
  Intrinsic<[], [llvm_i32_ty], []>;

def int_or1k_trap : GCCBuiltin<"__builtin_or1k_trap">,
  Intrinsic<[], [llvm_i32_ty], []>;

def int_or1k_rfe : GCCBuiltin<"__builtin_or1k_rfe">,
  Intrinsic<[], [], []>;

//===----------------------------------------------------------------------===//
// OR1K Move from/to special register intrinsics.
//===----------------------------------------------------------------------===//

def int_or1k_mfspr : GCCBuiltin<"__builtin_or1k_mfspr">,
  Intrinsic<[llvm_i32_ty], [llvm_i32_ty], []>;

def int_or1k_mtspr : GCCBuiltin<"__builtin_or1k_mtspr">,
  Intrinsic<[], [llvm_i32_ty, llvm_i32_ty], []>;

}
