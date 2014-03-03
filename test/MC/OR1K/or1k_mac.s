# RUN: llvm-mc -arch=or1k -show-encoding %s | FileCheck %s

	l.macrc r3
  # CHECK: # encoding: [0x18,0x61,0x00,0x00]

	l.mac r3, r4
  # CHECK: # encoding: [0xc4,0x03,0x20,0x01]

	l.macu r3, r4
  # CHECK: # encoding: [0xc4,0x03,0x20,0x03]

	l.maci r3, 42
  # CHECK: # encoding: [0x4c,0x03,0x00,0x2a]

	l.msb r3, r4
  # CHECK: # encoding: [0xc4,0x03,0x20,0x02]

	l.msbu r3, r4
  # CHECK: # encoding: [0xc4,0x03,0x20,0x04]
