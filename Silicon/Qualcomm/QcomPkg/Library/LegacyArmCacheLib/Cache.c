/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// Retained from edk2 e7aac7fc for SEC and SimpleInit's direct Linux handoff.
#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
typedef VOID (*AARCH64_CACHE_OPERATION)(UINTN);
VOID AArch64AllDataCachesOperation(AARCH64_CACHE_OPERATION Operation);
VOID EFIAPI ArmInvalidateDataCacheEntryBySetWay(UINTN SetWay);

VOID
AArch64DataCacheOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN  SavedInterruptState;

  SavedInterruptState = ArmGetInterruptState ();
  ArmDisableInterrupts ();

  AArch64AllDataCachesOperation (DataCacheOperation);

  ArmDataSynchronizationBarrier ();

  if (SavedInterruptState) {
    ArmEnableInterrupts ();
  }
}

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  )
{
  ASSERT (!ArmMmuEnabled ());

  ArmDataSynchronizationBarrier ();
  AArch64DataCacheOperation (ArmInvalidateDataCacheEntryBySetWay);
}
