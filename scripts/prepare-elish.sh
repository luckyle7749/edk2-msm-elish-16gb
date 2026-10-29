#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
case "${1:-}" in
  '')
    git submodule update --init Common/edk2 Common/edk2-platforms GPLDrivers/Library/SimpleInit Platform/EFI_Binaries tools/Installer
    # Only libraries used by the elish firmware, excluding upstream test suites.
    git -C Common/edk2 submodule update --init \
      BaseTools/Source/C/BrotliCompress/brotli \
      MdeModulePkg/Library/BrotliCustomDecompressLib/brotli \
      MdePkg/Library/BaseFdtLib/libfdt \
      MdePkg/Library/MipiSysTLib/mipisyst
    git -C GPLDrivers/Library/SimpleInit submodule update --init --recursive
    ;;
  --offline) ;;
  *) echo 'Usage: scripts/prepare-elish.sh [--offline]' >&2; exit 2;;
esac
while read -r revision module; do
  actual=$(git -C "$module" rev-parse HEAD)
  if [[ "$actual" != "$revision" ]]; then
    echo "Pinned revision mismatch: $module ($actual, expected $revision)" >&2
    exit 1
  fi
done < SUBMODULES.lock
patch_diff=$(mktemp)
trap 'rm -f "$patch_diff"' EXIT
apply_patch() {
  local module=$1 patch=$2
  git -C "$module" diff --cached --quiet || { echo "Staged submodule edits: $module" >&2; exit 1; }
  git -C "$module" diff --binary HEAD > "$patch_diff"
  if cmp -s "$patch_diff" "$repo_root/$patch"; then
    echo "Patch already applied: $module"
  elif [[ ! -s "$patch_diff" ]]; then
    git -C "$module" apply --check "$repo_root/$patch"
    git -C "$module" apply "$repo_root/$patch"
    echo "Applied: $patch"
  else
    echo "Unexpected edits in $module; preserve/review them before building." >&2
    exit 1
  fi
}
git -C Common/edk2 diff --quiet || { echo "Unexpected EDK2 core edits" >&2; exit 1; }
apply_patch GPLDrivers/Library/SimpleInit patches/simple-init-uefi.patch
