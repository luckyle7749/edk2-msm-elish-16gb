# EDK2 for Xiaomi Mi Pad 5 Pro (elish, 16GB RAM)

Community firmware based on [edk2-porting/edk2-msm](https://github.com/edk2-porting/edk2-msm), with a conservative 16GB RAM fix and SimpleInit ESP configuration fixes. This is a device-port fork, not Xiaomi or TianoCore-endorsed firmware.

## New-core migration branch

This branch migrates elish to official TianoCore **edk2-stable202608**,
commit `2970e5699ba6267f3384ffab20f96647578aebc8`.
The firmware builds locally, but device testing found an ESP-access regression:
Arch configuration is not loaded and ESP cannot be opened from the file manager.
Migration work is stopped; this branch preserves the work for later investigation.
Published v1.0.0 remains available as the known working rollback. See [migration notes](docs/EDK2-MIGRATION.md).

## Device-tested v1.0.0 baseline

- Device tested: **elish**, Snapdragon 870 / SM8250-AC, 16GB RAM.
- Upstream device-port commit: `e1952621f419f8db60ed28271264e1b5184c571d`.
- TianoCore EDK2 commit: `e7aac7fc137e247edad22f7ee53b9a1fba227397` (2023-01-27). This release deliberately retains the tested core; upgrading it is separate work.
- SMBIOS reports 16GiB; tested Linux reports **15354808 KiB (about 14.64GiB)** managed RAM.
- Original low-address platform map retained byte-for-byte. No 256MiB staging-area reclamation or small-gap reclamation is included.

The supplemental RAM is read from the bootloader device tree, not assumed solely from a device name. On the tested unit it is 2GiB at `0x200000000` and 8GiB at `0x800000000`. Fixed firmware reservations and existing mappings are excluded. New regions use allocated boot-services memory with writeback mapping, reclaimed by Linux after ExitBootServices.

## Clone and build

```sh
git clone -b upgrade/edk2-stable202608 https://github.com/luckyle7749/edk2-msm-elish-16gb.git
cd edk2-msm-elish-16gb
bash scripts/build-elish.sh
```

GitHub automatic source ZIP/tar downloads omit submodules; use `git clone` for the build instructions below.

The wrapper initializes pinned dependencies and applies the tracked submodule patches automatically. It checks revisions against `SUBMODULES.lock`, accepts the exact already-applied patches on subsequent builds, and stops on unexpected submodule edits. Do not use `git submodule update --remote` for a release build. After preparation, `git status` shows the patched SimpleInit submodule as modified; the exact changes are tracked in `patches/` and checked before each build.

On Debian/Ubuntu, install build dependencies first:

```sh
sudo apt-get install build-essential uuid-dev acpica-tools git nasm \
  python3 python-is-python3 gettext gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu \
  clang llvm lld device-tree-compiler libfdt-dev
```

For native Arch Linux hosts, the equivalents include `base-devel`, `git`, `python`, `gettext`, `acpica`, `nasm`, `clang`, `llvm`, `lld`, `dtc`, `aarch64-linux-gnu-gcc` and `aarch64-linux-gnu-binutils`.

Tested host tool versions and baseline-image provenance are in [docs/RELEASE-BASELINE.md](docs/RELEASE-BASELINE.md). Host BaseTools default to `gcc -std=gnu17`; target firmware uses the pinned core’s `CLANGDWARF` profile, Clang and LLVM LLD.

Output: **`boot-elish.img`** in the repository root. Ordinary rebuilds may differ in hash because the upstream build embeds version strings, paths and timestamps. The release SHA256 identifies the exact device-tested binary.

Optional host regression tests (requires the system libfdt development library):

```sh
bash scripts/test-high-memory.sh
```

After dependencies are initialized, `bash scripts/build-elish.sh --offline` skips dependency fetching. The upstream build expects to run from the repository root; the wrapper handles that automatically.

## ESP and Linux

This release contains UEFI firmware, **not** a Linux kernel, root filesystem, or disk partition layout. The tested ESP contains `Image`, `elish-csot.dtb`, and `simpleinit.uefi.cfg`; see [examples/simpleinit.uefi.cfg](examples/simpleinit.uefi.cfg). Supply a matching kernel and device tree separately, with a valid root-device command line.

Keep Arch boot settings in the ESP dynamic configuration. The firmware does not embed an Arch-specific kernel path/default/countdown patch. `ENABLE_LINUX_UTILS=1` remains enabled to match the device-tested baseline; the upstream tool payload/static defaults remain present.

## Flash and rollback

These commands describe the tested A/B setup: Android in slot A, UEFI in **boot_b**. Back up the current boot_b and verify device/slot before flashing. The commands do not repartition or format anything.

```sh
fastboot getvar product       # must be elish
fastboot getvar current-slot
fastboot flash boot_b boot-elish.img
fastboot set_active b
fastboot reboot
```

Return to Android boot slot A:

```sh
fastboot set_active a
fastboot reboot
```

Restore your own previously backed-up UEFI image:

```sh
fastboot flash boot_b YOUR_BACKUP.img
fastboot set_active b
fastboot reboot
```

On this setup, recovery requires slot A. When already in fastboot, select A, reboot to bootloader, then reboot recovery; enable ADB in recovery if needed. Never use a backup from a different device. `fastboot boot` was rejected on the tested unit.

## Device-tested v1.0.0 validation and known issues

- SimpleInit ESP dynamic configuration, default entry and countdown tested.
- SMBIOS 16GiB and both supplemental RAM banks verified after reboot.
- Four-pattern read/write test over 10GiB passed, physically covering the full 2GiB supplemental bank and about 7.34GiB of the supplemental 8GiB bank. This is a live-system test, not an exhaustive test of every physical page.
- Arch Linux boots to KDE Wayland with touch and Wi-Fi on the tested setup. These Linux features depend on the separate kernel/rootfs.
- **Reboot to Android menu payload is not working** on the tested setup. Use fastboot slot switching; the example ESP config omits that entry.
- Wi-Fi requires a Linux-side fixed MAC and iwd startup ordering on this unit; it is not provided by this firmware.
- Audio/DSP Linux firmware support remains incomplete; upgrading this UEFI image does not install those files.
- No modern-core migration, new Secure Boot assurance, or certification is claimed by this baseline.

## Changes and dependencies

[docs/CHANGES.md](docs/CHANGES.md) lists the retained patches and deliberately reverted experiments. Existing upstream licenses are retained, including the separate licenses in submodules and GPL components. See [the original README](docs/UPSTREAM-README.md) for upstream acknowledgements.
