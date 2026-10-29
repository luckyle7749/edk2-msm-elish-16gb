#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
# Optional host regression checks; Debian/Ubuntu package: libfdt-dev.
output=$(mktemp -d)
trap 'rm -rf "$output"' EXIT
"${CC:-cc}" -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DHIGH_MEMORY_HOST_TEST \
  -ISilicon/Qualcomm/QcomPkg/Library/MemoryInitPeiLib \
  tools/tests/test-high-memory.c \
  Silicon/Qualcomm/QcomPkg/Library/MemoryInitPeiLib/HighMemory.c \
  -lfdt -o "$output/test-high-memory"
"$output/test-high-memory" Platform/Xiaomi/sm8250/FdtBlob_compat/elish.dtb
