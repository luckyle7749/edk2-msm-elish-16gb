/** @file
  Validate bootloader RAM before exporting additional memory to the OS.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef QCOM_HIGH_MEMORY_H
#define QCOM_HIGH_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define HIGH_MEMORY_MAX_RANGES 32

typedef struct {
  uint64_t Base;
  uint64_t Size;
} HIGH_MEMORY_RANGE;

/* No allocations, HOB creation or physical memory accesses. Output is valid
   only on success; Count is always zero on failure. */
int PlanHighMemory (
  const void *Fdt, uint64_t Floor, unsigned int AddressBits,
  const HIGH_MEMORY_RANGE *Excluded, size_t ExcludedCount,
  HIGH_MEMORY_RANGE *Output, size_t Capacity, size_t *Count
  );

#endif
