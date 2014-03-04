# RUN: llvm-mc -arch=or1k --show-encoding %s | FileCheck %s

    l.mfspr r4, r0, 42
# CHECK: # encoding: [0xb4,0x80,0x00,0x2a]

	l.mtspr r0, r4, 8195
# CHECK: # encoding: [0xc0,0x80,0x20,0x03]
