//===- OR1KCallingConv.td - Calling Conventions OR1K -------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the OR1K architectures.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Return Value Calling Conventions
//===----------------------------------------------------------------------===//

// OR1K 32-bit C return-value convention.
def RetCC_OR1K32 : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R11, R12]>>,
  CCIfType<[f32], CCAssignToReg<[R11, R12]>>
]>;

class CCIfDefaultABI<CCAction A>
 : CCIf<"State.getTarget().getSubtarget<OR1KSubtarget>().isDefaultABI()", A> {}

class CCIfFixedArg<CCAction A>
 : CCIf<"!static_cast<OR1KCCState&>(State).isEndOfFixedArgs()", A> {}

def CC_OR1K32_NewABI : CallingConv<[
  // Special handling is required for an i64 argument. This is split into two
  // i32 elements and the ABI requires that both must be registers or stack
  // locations. The first i32 element is tagged as 'split'.

  // Split element: check to have enough registers for both elements and
  // allocate the first, otherwise shadow all registers and allocate on stack
  // the first element.
  CCIfSplit<CCIfType<[i32, f32], CCCustom<"CC_OR1K32_PairedArgs">>>,

  // Generic arguments are passed in registers if available.
  CCIfType<[i32, f32], CCAssignToReg<[R3, R4, R5, R6, R7, R8]>>,

  // Alternatively, they are assigned to the stack in 4-byte aligned units.
  CCAssignToStack<4, 4>
]>;

def CC_OR1K32_DefaultABI : CallingConv<[
  // Fixed arguments follow the same convention for both DefaultABI and NewABI.
  CCIfFixedArg<CCDelegateTo<CC_OR1K32_NewABI>>,

  // Variadic arguments are always passed on the stack.
  CCAssignToStack<4, 4>
]>;

// OR1K 32-bit C Calling convention.
def CC_OR1K32 : CallingConv<[
  // Promote i8/i16 args to i32
  CCIfType<[i8, i16], CCPromoteToType<i32>>,

  CCIfDefaultABI<CCDelegateTo<CC_OR1K32_DefaultABI>>,

  CCDelegateTo<CC_OR1K32_NewABI>
]>;

def CSR: CalleeSavedRegs<(add R30, R28, R26, R24, R22, R20, R18, R16, R14, R2)>;
