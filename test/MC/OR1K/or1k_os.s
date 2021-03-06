# RUN: llvm-mc -arch=or1k --show-encoding %s | FileCheck %s

    l.sys 42
# CHECK: # encoding: [0x20,0x00,0x00,0x2a]

	l.trap 42
# CHECK: # encoding: [0x21,0x00,0x00,0x2a]

	l.rfe
# CHECK: # encoding: [0x24,0x00,0x00,0x00]
