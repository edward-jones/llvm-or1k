# RUN: llvm-mc -arch=or1k -mattr=fbit -show-encoding %s | FileCheck %s

    l.ff1 r4, r3
# CHECK: # encoding: [0xe0,0x83,0x00,0x0f]

    l.fl1 r4, r3
# CHECK: # encoding: [0xe0,0x83,0x01,0x0f]
