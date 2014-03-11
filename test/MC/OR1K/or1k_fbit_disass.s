# RUN: llvm-mc -arch=or1k -mattr=fbit -disassemble %s | FileCheck %s

    0xe0 0x83 0x00 0x0f
# CHECK: l.ff1 r4, r3

    0xe0 0x83 0x01 0x0f
# CHECK: l.fl1 r4, r3
