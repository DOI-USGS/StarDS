/*
 * Minimal mbedTLS configuration for the StarDS WebAssembly build.
 *
 * The emscripten sysroot ships no OpenSSL, so under __EMSCRIPTEN__ the AWS SigV4
 * signing path (StarDS/include/stards.h, namespace s3crypto) uses mbedTLS for the
 * one primitive it needs from a crypto library: SHA-256. HMAC-SHA256 is hand-rolled
 * (RFC 2104) over this one-shot SHA-256, so the generic message-digest layer (md.c),
 * the other hashes, and the PSA crypto core are all left out. This keeps the vendored
 * mbedTLS footprint to exactly two source files (library/sha256.c + platform_util.c).
 *
 * Wired in via -DMBEDTLS_CONFIG_FILE="stards_mbedtls_config.h" in the top-level
 * CMakeLists EMSCRIPTEN branch. Native builds never see mbedTLS at all.
 */
#ifndef STARDS_MBEDTLS_CONFIG_H
#define STARDS_MBEDTLS_CONFIG_H

#define MBEDTLS_SHA256_C

#endif  // STARDS_MBEDTLS_CONFIG_H
