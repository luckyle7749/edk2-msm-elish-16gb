#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
prepare_args=()
if [[ "${1:-}" == --offline ]]; then prepare_args+=(--offline); shift; fi
bash scripts/prepare-elish.sh "${prepare_args[@]}"
for tool in git python3 make gcc clang aarch64-linux-gnu-gcc aarch64-linux-gnu-ld msgfmt; do
  command -v "$tool" >/dev/null || { echo "Missing build dependency: $tool" >&2; exit 1; }
done
# Legacy PCCTS uses pre-C23 function declarations. Scope the language mode
# to host BaseTools; target firmware still uses the selected EDK2 toolchain.
export BUILD_CC="${BUILD_CC:-gcc -std=gnu17}"
bash build.sh --device elish "$@"
