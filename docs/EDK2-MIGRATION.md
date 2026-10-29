# elish: migration to edk2-stable202608

Status: clean-checkout host build passed; device testing found an ESP-access regression. Migration work stopped at the user’s request.

Preservation branch: `migration/edk2-stable202608`.

## Source and scope

- Device port: e1952621f419f8db60ed28271264e1b5184c571d plus this fork’s patches.
- Official core: https://github.com/tianocore/edk2/tree/edk2-stable202608
- Core commit: 2970e5699ba6267f3384ffab20f96647578aebc8.
- SimpleInit and Qualcomm binary drivers retain their existing pinned revisions.
- This branch targets elish; other devices are not part of migration validation.
- Published v1.0.0 and its device-tested image remain the rollback baseline.

## Adaptations

Use the core’s own build-rule/toolchain templates and CLANGDWARF/LLD.
Resolve moved ArmLib, ArmSmcLib, compiler-intrinsics, ArmMmuLib, FdtLib and
interrupt-driver paths. Add the current stack-check, GPT and DXE image-record
libraries. Replace removed exception-handler libraries with the current DXE and
SEC/PEI implementations. Remove obsolete UGA dependencies; SimpleInit uses GOP.

Keep the SEC vector staging address at 0x9ff8c000 through a platform-owned PCD.
Remove the obsolete timer-frequency override (the current timer reads CNTFRQ).
Update processor-ID and cache-size SMBIOS initializers to the current structure
layout, preserving their stored values. Retain the existing 16GiB RAM report.

LegacyArmCacheLib contains the required set/way cache operations copied from the
previous core e7aac7fc, with original copyright and BSD license retained. New
ArmLib no longer exports these operations; SEC initialization and SimpleInit’s
direct Linux handoff still use them. Whole-cache CacheMaintenanceLib entry points
assert instead of implementing them, so they are not substitutes. No core patch
is required, and the earlier BaseTools warning cleanup patch is retired.

Keep the original low-address memory map and the two existing supplemental high
RAM banks. No low-memory reclamation is introduced. `NS_DEVICE` now maps to the
current DEVICE attribute in this non-secure firmware. Kernel, DTB, ESP boot
configuration and Arch root filesystem are separate from this core migration.

Build the upstream SwitchSlotsApp explicitly for elish, using EfiResetCold as
already tested with the old firmware. Automatic BootSlotDxe remains disabled.
The app must be explicitly invoked from an ESP entry; it is not run at startup.

## Build

Run `bash scripts/build-elish.sh`. The wrapper verifies SUBMODULES.lock and
applies the exact tracked SimpleInit patch. It fetches only the core libraries
needed by this elish build, not unrelated upstream test-suite dependencies.
Requires clang, llvm-ar, llvm-objcopy, ld.lld, GNU AArch64 tools and the other
README dependencies. The tested host uses Clang 23.1.3, LLD 23.1.4, GCC 16.2.0
and Python 3.14. No host packages were changed during this migration.

The FD retains base 0xce000000 and size 0x700000. The initial successful build
uses 6,207,592 bytes of the compressed 7MiB FV, leaving 1,132,440 bytes free.
Normal build metadata can change these sizes slightly.

## Validation

- Successful RELEASE build and Android boot-image packaging.
- Existing 18 ASan/UBSan high-memory boundary/layout checks passed.
- Clean-checkout build at 390aeedaa88719396d5127ff5ae7aee365a51a76 passed.
- Boot-image payload matches BootShim plus the complete FD; appended DTB is valid.
- FD reset-vector branch targets the SEC entry at offset 0x1410.
- Actual bootloader RAM fixture produces exactly the supplemental 2GiB and 8GiB banks.
- Device test: the menu appears, but the ESP Arch configuration is not loaded.
  Selecting ESP in the file manager has no visible effect. Root cause is unresolved.
- Arch boot, Linux memory-map comparison, Wi-Fi and cold reboot to Android slot A
  remain unverified with this core.
- Temporary SDIAG popup/console diagnostics have been removed from the source.
  This branch preserves the migration work and is not a working-release replacement.
- Flash only boot_b. Preserve the current working image and its SHA256.

The clean-build candidate is 7,137,280 bytes, SHA256:
`1b478e6c10d73dc170d16d2e38fdcd638f9deb01531b95b96f0c60998117449f`.
It was flashed for testing. The published v1.0.0 release remains unchanged.
The tablet was last flashed with a temporary diagnostic build; cleaning this
repository does not restore the tablet firmware.
