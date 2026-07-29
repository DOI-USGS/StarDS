#!/usr/bin/env bash
# Build StarDS/tests/wasm/read_s3_test.cpp to WebAssembly and run it under node,
# reading a real public .stards object over HTTPS via the emscripten fetch()
# backend. Requires emscripten (em++) and node >=18 on PATH (node >=18 ships a
# global fetch(); the WASM backend uses EM_ASYNC_JS + await fetch()).
#
# Usage:  StarDS/tests/wasm/run_read_s3_test.sh
set -euo pipefail

# Repo root (this script lives in StarDS/tests/wasm/).
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="${TMPDIR:-/tmp}/stards_wasm_read"
mkdir -p "$OUT"

EMXX="${EMXX:-em++}"
NODE="${NODE:-node}"

echo "== compiling read_s3_test.cpp -> wasm =="
"$EMXX" -std=c++20 -O2 \
  -DENABLE_CURL -DENABLE_ZLIB -DENABLE_S3 \
  -sUSE_ZLIB=1 \
  -sASYNCIFY \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXIT_RUNTIME=1 \
  -I "$ROOT/StarDS/include" \
  -I "$ROOT/submodules/mbedtls/include" \
  -I "$ROOT/cmake" \
  -DMBEDTLS_CONFIG_FILE='"stards_mbedtls_config.h"' \
  "$ROOT/StarDS/tests/wasm/read_s3_test.cpp" \
  "$ROOT/submodules/mbedtls/library/sha256.c" \
  "$ROOT/submodules/mbedtls/library/platform_util.c" \
  -o "$OUT/read_s3_test.js"

echo "== running under node =="
"$NODE" "$OUT/read_s3_test.js"
