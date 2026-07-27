// stards_config — report the build configuration of the StarDS tools.
//
// Prints the StarDS header version this binary was built against, which optional
// features were compiled in (zlib / lz4 / curl / S3), the on-disk format version,
// and the default StarConfig/OpenOptions used when creating/opening a dataset.
// Because it is compiled from the same header with the same feature macros as the
// other tools, its output reflects exactly what `stardsls` / `stards_translate`
// (and this build's library) will do.
//
// Usage:
//   stards_config            Human-readable report (default)
//   stards_config --json     Machine-readable JSON (for scripts / CI)
//   stards_config --version  Just the version string
//   stards_config -h|--help  This help

#include "../StarDS/include/stards.h"

#include <cstring>
#include <iostream>
#include <string>

using namespace star;

// Human-readable name for a compression algorithm (mirrors stardsls.cpp).
static const char* compression_name(CompressionAlgorithm c) {
    switch (c) {
        case CompressionAlgorithm::NONE:         return "none";
        case CompressionAlgorithm::GZIP:         return "gzip";
        case CompressionAlgorithm::ZSTD:         return "zstd";
        case CompressionAlgorithm::LZ4:          return "lz4";
        case CompressionAlgorithm::GZIP_SHUFFLE: return "gzip-shuffle";
        case CompressionAlgorithm::LZ4_SHUFFLE:  return "lz4-shuffle";
        default:                                 return "unknown";
    }
}

// Feature flags are compile-time; capture them once as bools so both the text and
// JSON paths report identically.
namespace {
#ifdef ENABLE_ZLIB
    constexpr bool kZlib = true;
#else
    constexpr bool kZlib = false;
#endif
#ifdef ENABLE_LZ4
    constexpr bool kLz4 = true;
#else
    constexpr bool kLz4 = false;
#endif
#ifdef ENABLE_CURL
    constexpr bool kCurl = true;
#else
    constexpr bool kCurl = false;
#endif
#ifdef ENABLE_S3
    constexpr bool kS3 = true;
#else
    constexpr bool kS3 = false;
#endif

// Platform / compiler identification for the build banner.
const char* platform_name() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "unknown";
#endif
}

std::string compiler_name() {
#if defined(__clang__)
    return "Clang " __clang_version__;
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
#elif defined(__GNUC__)
    return "GCC " __VERSION__;
#else
    return "unknown";
#endif
}
}  // namespace

static void print_help(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [--json | --version | -h]\n\n"
        "Report the build configuration of the StarDS tools:\n"
        "  * StarDS header version this binary was built against\n"
        "  * Which optional features were compiled in (zlib/lz4/curl/S3)\n"
        "  * The on-disk format version\n"
        "  * The default settings applied when creating a new .stards file\n\n"
        "Options:\n"
        "  --json       Emit the same information as JSON (for scripts/CI)\n"
        "  --version    Print just the version string and exit\n"
        "  -h, --help   Show this help\n";
}

static void print_text() {
    // A default-constructed config/header is exactly what create()/open() use.
    StarConfig cfg;
    OpenOptions oo;
    FileHeader hdr;  // default format_version

    std::cout << "StarDS build configuration\n";
    std::cout << "==========================\n";
    std::cout << "  Version:         " << getLibraryVersion() << "\n";
    std::cout << "  Format version:  v" << static_cast<int>(hdr.format_version)
              << "  (magic \"" << MAGIC_STRING << "\")\n";
    std::cout << "  Platform:        " << platform_name() << "\n";
    std::cout << "  Compiler:        " << compiler_name() << "\n";
    std::cout << "  C++ standard:    " << __cplusplus << "\n";

    std::cout << "\nEnabled features\n";
    std::cout << "----------------\n";
    std::cout << "  zlib (gzip):     " << (kZlib ? "yes" : "no") << "\n";
    std::cout << "  lz4:             " << (kLz4  ? "yes" : "no") << "\n";
    std::cout << "  curl (HTTP):     " << (kCurl ? "yes" : "no") << "\n";
    std::cout << "  S3 (OpenSSL):    " << (kS3   ? "yes" : "no") << "\n";

    std::cout << "\nDefaults for a new dataset (StarConfig)\n";
    std::cout << "---------------------------------------\n";
    std::cout << "  compression:            " << compression_name(cfg.compression) << "\n";
    std::cout << "  block_size:             " << cfg.block_size << " bytes\n";
    std::cout << "  metadata_block_enabled: " << (cfg.metadata_block_enabled ? "true" : "false") << "\n";
    std::cout << "  metadata_compression:   " << compression_name(cfg.metadata_compression) << "\n";
    std::cout << "  metadata_max_block:     " << cfg.metadata_max_block_size << " bytes\n";
    std::cout << "  buffer_shrink_thresh:   " << cfg.buffer_shrink_threshold << " bytes\n";
    std::cout << "  arena_chunk_size:       " << cfg.arena_chunk_size << " bytes\n";

    std::cout << "\nDefaults when opening a dataset (OpenOptions)\n";
    std::cout << "---------------------------------------------\n";
    std::cout << "  layer_inheritance:          " << (oo.layer_inheritance ? "true" : "false") << "\n";
    std::cout << "  prefetch_whole_below_bytes: " << oo.prefetch_whole_below_bytes << " bytes\n";

    // Note the sliceability consequence of the default codec, since it surprises
    // users who expect get_slice() to work on a freshly created dataset.
    if (uses_shuffle(cfg.compression)) {
        std::cout << "\nNote: the default codec (" << compression_name(cfg.compression)
                  << ") is a byte-shuffle codec, which is NOT sliceable. Create with an\n"
                     "explicit non-shuffle codec (none/gzip/lz4) if you need get_slice().\n";
    }
}

static void print_json() {
    StarConfig cfg;
    OpenOptions oo;
    FileHeader hdr;

    auto b = [](bool v) { return v ? "true" : "false"; };

    std::cout << "{\n";
    std::cout << "  \"version\": \"" << getLibraryVersion() << "\",\n";
    std::cout << "  \"format_version\": " << static_cast<int>(hdr.format_version) << ",\n";
    std::cout << "  \"magic\": \"" << MAGIC_STRING << "\",\n";
    std::cout << "  \"platform\": \"" << platform_name() << "\",\n";
    std::cout << "  \"compiler\": \"" << compiler_name() << "\",\n";
    std::cout << "  \"cpp_standard\": " << __cplusplus << ",\n";
    std::cout << "  \"features\": {\n";
    std::cout << "    \"zlib\": " << b(kZlib) << ",\n";
    std::cout << "    \"lz4\": "  << b(kLz4)  << ",\n";
    std::cout << "    \"curl\": " << b(kCurl) << ",\n";
    std::cout << "    \"s3\": "   << b(kS3)   << "\n";
    std::cout << "  },\n";
    std::cout << "  \"defaults\": {\n";
    std::cout << "    \"compression\": \"" << compression_name(cfg.compression) << "\",\n";
    std::cout << "    \"block_size\": " << cfg.block_size << ",\n";
    std::cout << "    \"metadata_block_enabled\": " << b(cfg.metadata_block_enabled) << ",\n";
    std::cout << "    \"metadata_compression\": \"" << compression_name(cfg.metadata_compression) << "\",\n";
    std::cout << "    \"metadata_max_block_size\": " << cfg.metadata_max_block_size << ",\n";
    std::cout << "    \"buffer_shrink_threshold\": " << cfg.buffer_shrink_threshold << ",\n";
    std::cout << "    \"arena_chunk_size\": " << cfg.arena_chunk_size << ",\n";
    std::cout << "    \"layer_inheritance\": " << b(oo.layer_inheritance) << ",\n";
    std::cout << "    \"prefetch_whole_below_bytes\": " << oo.prefetch_whole_below_bytes << ",\n";
    std::cout << "    \"default_codec_sliceable\": " << b(!uses_shuffle(cfg.compression)) << "\n";
    std::cout << "  }\n";
    std::cout << "}\n";
}

int main(int argc, char* argv[]) {
    // Keep this tool quiet: no library log spew on stderr.
    logger::set_log_level(logger::STARDS_ERROR);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        }
        if (arg == "--version") {
            std::cout << getLibraryVersion() << "\n";
            return 0;
        }
        if (arg == "--json") {
            print_json();
            return 0;
        }
        std::cerr << "Unknown option: " << arg << "\n";
        print_help(argv[0]);
        return 1;
    }

    print_text();
    return 0;
}
