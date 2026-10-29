#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2017, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
# Copyright (c) 2020, NUVIA Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

// Retained from edk2 e7aac7fc for the elish direct Linux handoff.
#include <AsmMacroLib.h>

ASM_FUNC(ArmInvalidateDataCacheEntryBySetWay)
  dc      isw, x0     // Invalidate this line
  ret



ASM_FUNC(ArmCleanDataCacheEntryBySetWay)
  dc      csw, x0     // Clean this line
  ret



ASM_FUNC(AArch64AllDataCachesOperation)
// We can use regs 0-7 and 9-15 without having to save/restore.
// Save our link register on the stack. - The stack must always be quad-word aligned
  stp   x29, x30, [sp, #-16]!
  mov   x29, sp
  mov   x1, x0                  // Save Function call in x1
  mrs   x6, clidr_el1           // Read EL1 CLIDR
  and   x3, x6, #0x7000000      // Mask out all but Level of Coherency (LoC)
  lsr   x3, x3, #23             // Left align cache level value - the level is shifted by 1 to the
                                // right to ease the access to CSSELR and the Set/Way operation.
  cbz   x3, L_Finished          // No need to clean if LoC is 0
  mov   x10, #0                 // Start clean at cache level 0

Loop1:
  add   x2, x10, x10, lsr #1    // Work out 3x cachelevel for cache info
  lsr   x12, x6, x2             // bottom 3 bits are the Cache type for this level
  and   x12, x12, #7            // get those 3 bits alone
  cmp   x12, #2                 // what cache at this level?
  b.lt  L_Skip                  // no cache or only instruction cache at this level
  msr   csselr_el1, x10         // write the Cache Size selection register with current level (CSSELR)
  isb                           // isb to sync the change to the CacheSizeID reg
  mrs   x12, ccsidr_el1         // reads current Cache Size ID register (CCSIDR)
  and   x2, x12, #0x7           // extract the line length field
  add   x2, x2, #4              // add 4 for the line length offset (log2 16 bytes)
  mov   x4, #0x400
  sub   x4, x4, #1
  and   x4, x4, x12, lsr #3     // x4 is the max number on the way size (right aligned)
  clz   w5, w4                  // w5 is the bit position of the way size increment
  mov   x7, #0x00008000
  sub   x7, x7, #1
  and   x7, x7, x12, lsr #13    // x7 is the max number of the index size (right aligned)

Loop2:
  mov   x9, x4                  // x9 working copy of the max way size (right aligned)

Loop3:
  lsl   x11, x9, x5
  orr   x0, x10, x11            // factor in the way number and cache number
  lsl   x11, x7, x2
  orr   x0, x0, x11             // factor in the index number

  blr   x1                      // Goto requested cache operation

  subs  x9, x9, #1              // decrement the way number
  b.ge  Loop3
  subs  x7, x7, #1              // decrement the index
  b.ge  Loop2
L_Skip:
  add   x10, x10, #2            // increment the cache number
  cmp   x3, x10
  b.gt  Loop1

L_Finished:
  dsb   sy
  isb
  ldp   x29, x30, [sp], #0x10
  ret
