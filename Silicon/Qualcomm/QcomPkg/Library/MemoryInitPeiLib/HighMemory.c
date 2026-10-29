/** @file
  Build a bounded, fail-closed plan for RAM above a platform's static map.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "HighMemory.h"
#ifdef HIGH_MEMORY_HOST_TEST
#include <libfdt.h>
#else
#include <comp_libfdt.h>
#endif

static int ValidRange (HIGH_MEMORY_RANGE R, uint64_t Limit)
{
  return R.Size != 0 && R.Base < Limit && R.Size <= Limit - R.Base;
}

static uint64_t ReadCells (const fdt32_t *Cells)
{
  return ((uint64_t)fdt32_to_cpu (Cells[0]) << 32) |
         fdt32_to_cpu (Cells[1]);
}

/* Exclusion rounds outward to whole pages, so even a partially reserved page
   can never be handed to Linux. The caller discards the plan on any error. */
static int Subtract (
  HIGH_MEMORY_RANGE *Ranges, size_t *Count, size_t Capacity,
  HIGH_MEMORY_RANGE Reserved, uint64_t Limit
  )
{
  size_t I;
  uint64_t Start, End, Left, Right;

  if (!ValidRange (Reserved, Limit))
    return -FDT_ERR_BADVALUE;
  Start = Reserved.Base & ~(uint64_t)4095;
  End = Reserved.Base + Reserved.Size;
  if (End & 4095) {
    if (End > Limit - 4095)
      return -FDT_ERR_BADVALUE;
    End = (End + 4095) & ~(uint64_t)4095;
  }
  for (I = 0; I < *Count; ) {
    Left = Ranges[I].Base;
    Right = Left + Ranges[I].Size;
    if (End <= Left || Start >= Right) {
      I++;
      continue;
    }
    if (Start <= Left && End >= Right) {
      Ranges[I] = Ranges[--(*Count)];
      continue;
    }
    if (Start > Left && End < Right) {
      if (*Count >= Capacity)
        return -FDT_ERR_NOSPACE;
      Ranges[*Count].Base = End;
      Ranges[(*Count)++].Size = Right - End;
      Ranges[I].Size = Start - Left;
    } else if (Start <= Left) {
      Ranges[I].Base = End;
      Ranges[I].Size = Right - End;
    } else {
      Ranges[I].Size = Start - Left;
    }
    I++;
  }
  return 0;
}

static int ExcludeProperty (
  const fdt32_t *Reg, int Length, HIGH_MEMORY_RANGE *Ranges,
  size_t *Count, size_t Capacity, uint64_t Limit
  )
{
  int I, Status;
  HIGH_MEMORY_RANGE R;
  if (Length <= 0 || Length % 16 != 0)
    return -FDT_ERR_BADVALUE;
  for (I = 0; I < Length / 4; I += 4) {
    R.Base = ReadCells (Reg + I);
    R.Size = ReadCells (Reg + I + 2);
    Status = Subtract (Ranges, Count, Capacity, R, Limit);
    if (Status != 0)
      return Status;
  }
  return 0;
}

int PlanHighMemory (
  const void *Fdt, uint64_t Floor, unsigned int AddressBits,
  const HIGH_MEMORY_RANGE *Excluded, size_t ExcludedCount,
  HIGH_MEMORY_RANGE *Output, size_t Capacity, size_t *Count
  )
{
  HIGH_MEMORY_RANGE Banks[HIGH_MEMORY_MAX_RANGES], R, Temp;
  size_t Used = 0, BankCount, I, J;
  uint64_t Limit, Start, End;
  int Node, Child, Length, Status, Entries;
  const fdt32_t *Reg;
  const char *Type;

  *Count = 0;
  if (!Fdt || !Output || (ExcludedCount && !Excluded) || Capacity == 0 ||
      AddressBits < 36 || AddressBits > 48 || (Floor & 4095))
    return -FDT_ERR_BADVALUE;
  Limit = (uint64_t)1 << AddressBits;
  if (Floor >= Limit || fdt_check_header (Fdt) != 0 ||
      fdt_address_cells (Fdt, 0) != 2 || fdt_size_cells (Fdt, 0) != 2)
    return -FDT_ERR_BADVALUE;

  /* Qualcomm's bootloader publishes a single /memory node. Reject additional
     memory nodes instead of silently ignoring part of an unfamiliar layout. */
  Node = fdt_path_offset (Fdt, "/memory");
  if (Node < 0)
    return Node;
  Reg = fdt_getprop (Fdt, Node, "reg", &Length);
  if (!Reg || Length <= 0 || Length % 16 != 0)
    return -FDT_ERR_BADVALUE;
  BankCount = (size_t)Length / 16;
  if (BankCount > HIGH_MEMORY_MAX_RANGES)
    return -FDT_ERR_NOSPACE;
  for (I = 0; I < BankCount; I++) {
    Banks[I].Base = ReadCells (Reg + I * 4);
    Banks[I].Size = ReadCells (Reg + I * 4 + 2);
    if (!ValidRange (Banks[I], Limit) ||
        ((Banks[I].Base | Banks[I].Size) & 4095))
      return -FDT_ERR_BADVALUE;
    for (J = 0; J < I; J++) {
      if (Banks[I].Base < Banks[J].Base + Banks[J].Size &&
          Banks[J].Base < Banks[I].Base + Banks[I].Size)
        return -FDT_ERR_BADVALUE;
    }
  }
  for (Child = fdt_first_subnode (Fdt, 0); Child >= 0;
       Child = fdt_next_subnode (Fdt, Child)) {
    Type = fdt_getprop (Fdt, Child, "device_type", &Length);
    if (Child != Node && Type && Length == 7 &&
        Type[0] == 'm' && Type[1] == 'e' && Type[2] == 'm' &&
        Type[3] == 'o' && Type[4] == 'r' && Type[5] == 'y' && Type[6] == 0)
      return -FDT_ERR_BADVALUE;
  }
  if (Child != -FDT_ERR_NOTFOUND)
    return Child;
  for (I = 0; I < BankCount; I++) {
    Start = Banks[I].Base > Floor ? Banks[I].Base : Floor;
    End = Banks[I].Base + Banks[I].Size;
    if (End <= Start)
      continue;
    if (Used >= Capacity)
      return -FDT_ERR_NOSPACE;
    Output[Used].Base = Start;
    Output[Used++].Size = End - Start;
  }
  for (I = 0; I < ExcludedCount; I++) {
    Status = Subtract (Output, &Used, Capacity, Excluded[I], Limit);
    if (Status != 0)
      return Status;
  }
  Entries = fdt_num_mem_rsv (Fdt);
  if (Entries < 0)
    return Entries;
  for (Node = 0; Node < Entries; Node++) {
    if (fdt_get_mem_rsv (Fdt, Node, &R.Base, &R.Size) != 0)
      return -FDT_ERR_BADVALUE;
    Status = Subtract (Output, &Used, Capacity, R, Limit);
    if (Status != 0)
      return Status;
  }
  Node = fdt_path_offset (Fdt, "/reserved-memory");
  if (Node >= 0) {
    if (fdt_address_cells (Fdt, Node) != 2 || fdt_size_cells (Fdt, Node) != 2)
      return -FDT_ERR_BADVALUE;
    Reg = fdt_getprop (Fdt, Node, "ranges", &Length);
    if ((Reg && Length != 0) || (!Reg && Length != -FDT_ERR_NOTFOUND))
      return -FDT_ERR_BADVALUE;
    for (Child = fdt_first_subnode (Fdt, Node); Child >= 0;
         Child = fdt_next_subnode (Fdt, Child)) {
      Reg = fdt_getprop (Fdt, Child, "reg", &Length);
      if (!Reg && Length == -FDT_ERR_NOTFOUND) {
        /* A dynamic pool has no fixed address yet. Require bounded allocation
           windows so unfamiliar unbounded pools still require review. */
        if (!fdt_getprop (Fdt, Child, "size", &Length))
          return -FDT_ERR_BADVALUE;
        Reg = fdt_getprop (Fdt, Child, "alloc-ranges", &Length);
        if (!Reg || Length <= 0 || Length % 16 != 0)
          return -FDT_ERR_BADVALUE;
        /* alloc-ranges bounds a future OS allocation; it is not an existing
           physical reservation. Validate the window but do not subtract it. */
        for (int Offset = 0; Offset < Length / 4; Offset += 4) {
          R.Base = ReadCells (Reg + Offset);
          R.Size = ReadCells (Reg + Offset + 2);
          if (!ValidRange (R, Limit))
            return -FDT_ERR_BADVALUE;
        }
        continue;
      }
      if (!Reg)
        return -FDT_ERR_BADVALUE;
      Status = ExcludeProperty (Reg, Length, Output, &Used, Capacity, Limit);
      if (Status != 0)
        return Status;
    }
    if (Child != -FDT_ERR_NOTFOUND)
      return Child;
  } else if (Node != -FDT_ERR_NOTFOUND) {
    return Node;
  }
  for (I = 0; I < Used; I++) {
    for (J = I + 1; J < Used; J++) {
      if (Output[J].Base < Output[I].Base) {
        Temp = Output[I]; Output[I] = Output[J]; Output[J] = Temp;
      }
    }
  }
  *Count = Used;
  return 0;
}
