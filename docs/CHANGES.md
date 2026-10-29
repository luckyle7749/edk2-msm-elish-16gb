# Retained changes

1. `MemoryInitPeiLib/HighMemory.c/.h`: bounded, allocation-free planning of bootloader RAM additions; validate RAM sizes/alignment/overlap/address width/capacity, subtract existing descriptors, fixed DT reservations and FDT memreserve entries. Bounded dynamic `alloc-ranges` describe future allocation windows, not already-occupied memory.
2. `MemoryInitPeiLib.c`, package DEC/INF, `elish.dsc`: opt-in supplementation above physical `0x200000000`; add resource/allocation HOBs and MMU mappings as writeback boot-services data. Other platforms default to disabled.
3. `PlatformSmbiosDxe`: for opted-in platforms, sum bootloader RAM and round to whole GiB to restore the installed-capacity label, updating SMBIOS Type 16 and Type 17. Usable RAM accounting remains in the EFI map.
4. `patches/simple-init-uefi.patch`: correct config volume-handle ownership, configuration save filename and fallback handling; guard `free(NULL)` before pool-header pointer arithmetic.
5. `patches/edk2-basetools.patch`: remove an unused counter without changing string scanning. The build wrapper selects a pre-C23 host C dialect for legacy PCCTS.
6. `ENABLE_LINUX_UTILS=1`: retained upstream tool packaging used by the tested baseline. It is optional functionality, retained pending a separate slim-build test.
7. Build wrappers, pinned dependency manifest and host regression tests.

Submodules retain their upstream gitlink commits. Their local changes are recorded as patch files in this repository; a recursive clone alone does not apply those patches. `scripts/build-elish.sh` performs that step.

# Deliberately excluded

- Low-address platform map changes: 256MiB staging reclamation, 87MiB/99MiB holes, smaller holes and PIL gap reclamation were all reverted.
- No global removal of CMake `-Werror`.
- No Arch-specific embedded default entry or forced autoboot C patch.
- No temporary FDT/memory-map export code or memory-test kernel module.
- No Linux kernel, rootfs, Wi-Fi secrets, SSH keys, partition backups, GPT, persist or boot_a images.
