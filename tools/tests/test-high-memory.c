/* SPDX-License-Identifier: BSD-2-Clause-Patent */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfdt.h>
#include "HighMemory.h"

static char Blob[65536];
static HIGH_MEMORY_RANGE Out[32];
static size_t Count;
static int Checks;

static void PutRange(int Node, const char *Name, uint64_t Base, uint64_t Size)
{
  fdt32_t R[4] = {cpu_to_fdt32(Base >> 32), cpu_to_fdt32(Base),
                 cpu_to_fdt32(Size >> 32), cpu_to_fdt32(Size)};
  assert(fdt_appendprop(Blob, Node, Name, R, sizeof(R)) == 0);
}

static int NewTree(void)
{
  int N;
  assert(fdt_create_empty_tree(Blob, sizeof(Blob)) == 0);
  assert(fdt_setprop_u32(Blob, 0, "#address-cells", 2) == 0);
  assert(fdt_setprop_u32(Blob, 0, "#size-cells", 2) == 0);
  N = fdt_add_subnode(Blob, 0, "memory@80000000");
  assert(N >= 0);
  assert(fdt_setprop_string(Blob, N, "device_type", "memory") == 0);
  return N;
}

static int Plan(void)
{
  return PlanHighMemory(Blob, 0x200000000ULL, 48, NULL, 0, Out, 32, &Count);
}

static void Good(size_t Expected)
{
  assert(Plan() == 0 && Count == Expected);
  Checks++;
}

static void Bad(void)
{
  Count = 99;
  assert(Plan() != 0 && Count == 0);
  Checks++;
}

static void RealFile(const char *Path)
{
  FILE *F = fopen(Path, "rb");
  long Length;
  void *Data;
  int Status;
  assert(F);
  assert(fseek(F, 0, SEEK_END) == 0);
  Length = ftell(F);
  assert(Length > 0 && Length < 8 * 1024 * 1024);
  rewind(F);
  Data = malloc((size_t)Length);
  assert(Data && fread(Data, 1, Length, F) == (size_t)Length);
  fclose(F);
  Status = PlanHighMemory(Data, 0x200000000ULL, 48, NULL, 0, Out, 32, &Count);
  printf("%s: status=%d count=%zu\n", Path, Status, Count);
  assert(Status == 0);
  for (size_t I = 0; I < Count; I++)
    printf("  0x%llx + 0x%llx\n", (unsigned long long)Out[I].Base,
           (unsigned long long)Out[I].Size);
  free(Data);
}

int main(int Argc, char **Argv)
{
  int N, P;
  HIGH_MEMORY_RANGE Excluded = {0x210000000ULL, 0x10000000};

  N = NewTree(); PutRange(N, "reg", 0x80000000, 0x180000000ULL);
  Good(0); /* 6 GiB */
  N = NewTree(); PutRange(N, "reg", 0x80000000, 0x200000000ULL);
  Good(1); assert(Out[0].Size == 0x80000000); /* 8 GiB */
  N = NewTree();
  PutRange(N, "reg", 0x80000000, 0x37700000);
  PutRange(N, "reg", 0x800000000ULL, 0x200000000ULL);
  PutRange(N, "reg", 0xc0000000, 0x1c0000000ULL);
  Good(2); /* actual 16 GiB layout, unsorted, with a large hole */
  assert(Out[0].Base == 0x200000000ULL && Out[0].Size == 0x80000000);
  assert(Out[1].Base == 0x800000000ULL && Out[1].Size == 0x200000000ULL);
  assert(PlanHighMemory(Blob, 0x200000000ULL, 48, &Excluded, 1, Out, 32, &Count) == 0);
  assert(Count == 3 && Out[0].Size == 0x10000000 && Out[1].Base == 0x220000000ULL);
  Checks++;
  assert(PlanHighMemory(Blob, 0x200000000ULL, 48, NULL, 0, Out, 1, &Count) != 0 && Count == 0);
  Checks++;
  assert(fdt_add_mem_rsv(Blob, 0x800000000ULL, 0x200000000ULL) == 0);
  Good(1);

  N = NewTree(); PutRange(N, "reg", 0x200000000ULL, 0x80000000);
  P = fdt_add_subnode(Blob, 0, "reserved-memory");
  assert(fdt_setprop_u32(Blob, P, "#address-cells", 2) == 0);
  assert(fdt_setprop_u32(Blob, P, "#size-cells", 2) == 0);
  assert(fdt_setprop(Blob, P, "ranges", NULL, 0) == 0);
  N = fdt_add_subnode(Blob, P, "test@210000001");
  PutRange(N, "reg", 0x210000001ULL, 1);
  Good(2); /* reserve even a partial page */
  assert(Out[1].Base == 0x210001000ULL);
  assert(fdt_delprop(Blob, N, "reg") == 0);
  assert(fdt_setprop_u64(Blob, N, "size", 4096) == 0);
  Bad(); /* unbounded dynamic reservation */
  PutRange(N, "alloc-ranges", 0, 0xffffffff);
  Good(1); /* dynamic allocation limited to low RAM */
  assert(fdt_delprop(Blob, N, "alloc-ranges") == 0);
  PutRange(N, "alloc-ranges", 0x200000000ULL, 0x80000000);
  Good(1); /* an allocation window is not an occupied reservation */


  N = NewTree(); PutRange(N, "reg", 0x200000000ULL, 0x80000000);
  PutRange(N, "reg", 0x210000000ULL, 0x80000000); Bad();
  N = NewTree(); PutRange(N, "reg", 0x200000001ULL, 0x1000); Bad();
  N = NewTree(); PutRange(N, "reg", 0x200000000ULL, 0); Bad();
  N = NewTree(); PutRange(N, "reg", UINT64_MAX - 4095, 8192); Bad();
  N = NewTree(); PutRange(N, "reg", 1ULL << 48, 4096); Bad();
  N = NewTree(); PutRange(N, "reg", 0x200000000ULL, 0x1000);
  assert(fdt_appendprop_u32(Blob, N, "reg", 0) == 0); Bad();
  N = NewTree(); PutRange(N, "reg", 0x200000000ULL, 0x1000);
  N = fdt_add_subnode(Blob, 0, "memory@900000000");
  assert(fdt_setprop_string(Blob, N, "device_type", "memory") == 0); Bad();
  N = NewTree(); PutRange(N, "reg", 0x200000000ULL, 0x1000);
  memset(Blob, 0, 4); Bad();
  printf("PASS: %d boundary and layout checks\n", Checks);
  for (int I = 1; I < Argc; I++) RealFile(Argv[I]);
  return 0;
}
