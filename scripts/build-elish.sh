#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
prepare_args=()
if [[ "${1:-}" == --offline ]]; then prepare_args+=(--offline); shift; fi
bash scripts/prepare-elish.sh "${prepare_args[@]}"
for tool in git python3 make gcc clang aarch64-linux-gnu-gcc aarch64-linux-gnu-ld llvm-ar llvm-objcopy ld.lld msgfmt; do
  command -v "$tool" >/dev/null || { echo "Missing build dependency: $tool" >&2; exit 1; }
done
# Keep the host tools language mode stable across GCC releases.
export BUILD_CC="${BUILD_CC:-gcc -std=gnu17}"
bash build.sh --device elish "$@"
