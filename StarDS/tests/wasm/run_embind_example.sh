#!/usr/bin/env bash
# Build the StarDS embind module (stards_embind.cpp) to a WebAssembly ES module
# and run the JS example against it under node.
#
# Produces build-embind/stards.mjs (+ .wasm). Usage:
#   StarDS/tests/wasm/run_embind_example.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="$ROOT/build-embind"
mkdir -p "$OUT"

EMXX="${EMXX:-em++}"
NODE="${NODE:-node}"

echo "== building embind module -> $OUT/stards.mjs =="
"$EMXX" -std=c++20 -O2 \
  --bind \
  -DENABLE_CURL -DENABLE_ZLIB -DENABLE_S3 \
  -sUSE_ZLIB=1 \
  -sASYNCIFY \
  -sASYNCIFY_IMPORTS=stards_em_fetch \
  -sALLOW_MEMORY_GROWTH=1 \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sENVIRONMENT=web,node \
  -I "$ROOT/StarDS/include" \
  -I "$ROOT/submodules/mbedtls/include" \
  -I "$ROOT/cmake" \
  -DMBEDTLS_CONFIG_FILE='"stards_mbedtls_config.h"' \
  "$ROOT/StarDS/tests/wasm/stards_embind.cpp" \
  "$ROOT/submodules/mbedtls/library/sha256.c" \
  "$ROOT/submodules/mbedtls/library/platform_util.c" \
  -o "$OUT/stards.mjs"

echo "== running example.mjs under node =="
"$NODE" "$ROOT/StarDS/tests/wasm/example.mjs"
