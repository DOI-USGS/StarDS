# Installation

StarDS is a **header-only C++ library** with optional CLI tools and Python
bindings. All of these are built from source with CMake.

## Requirements

| Component | Requirement |
|-----------|-------------|
| C++ compiler | C++20 (or newer) |
| CMake | 3.10+ (3.14+ for Python bindings) |
| Python (bindings) | 3.8+ |
| NumPy (bindings) | 1.20.0+ |
| SWIG (bindings) | 4.0+ |

**Optional native dependencies** (enable extra features):

- **zlib** — GZIP compression (build flag `STAR_ENABLE_ZLIB`)
- **LZ4** — fast compression codec (build flag `STAR_ENABLE_LZ4`)
- **libcurl** — HTTP remote access (build flag `STAR_ENABLE_CURL`)
- **OpenSSL** — S3 request signing (build flag `STAR_ENABLE_S3`)

## Clone the repository

```bash
git clone https://code.usgs.gov/astrogeology/stards.git
cd stards
git submodule update --init --recursive
```

## Build the CLI tools

```bash
mkdir build && cd build
cmake .. \
  -DSTAR_BUILD_TOOLS=ON \
  -DSTAR_ENABLE_ZLIB=ON \
  -DSTAR_ENABLE_LZ4=ON \
  -DSTAR_ENABLE_CURL=ON \
  -DSTAR_ENABLE_S3=ON

make -j$(nproc)
```

This builds [`starls`](../cli/starls.md) and
[`star_translate`](../cli/star-translate.md).

### CMake options

All options default to `ON`, so a plain `cmake ..` already builds the library,
tools, and (where SWIG/Python are available) the bindings with every optional
feature enabled. Pass `-D<OPTION>=OFF` to turn a feature off.

| Option | Default | Effect |
|--------|---------|--------|
| `STAR_BUILD_LIB=ON` | ON | Build the header-only library target |
| `STAR_BUILD_TOOLS=ON` | ON | Build the CLI tools (`starls`, `star_translate`) |
| `STAR_BUILD_TESTS=ON` | ON | Build the unit tests |
| `STAR_BUILD_PYTHON_BINDINGS=ON` | ON | Build the Python bindings |
| `STAR_ENABLE_ZLIB=ON` | ON | Enable GZIP compression |
| `STAR_ENABLE_LZ4=ON` | ON | Enable LZ4 compression |
| `STAR_ENABLE_CURL=ON` | ON | Enable HTTP remote access |
| `STAR_ENABLE_S3=ON` | ON | Enable S3 cloud storage (requires CURL) |

## Build the Python bindings

```bash
mkdir build && cd build
cmake .. \
  -DSTAR_BUILD_PYTHON_BINDINGS=ON \
  -DSTAR_ENABLE_ZLIB=ON \
  -DSTAR_ENABLE_LZ4=ON \
  -DSTAR_ENABLE_CURL=ON \
  -DSTAR_ENABLE_S3=ON

make -j$(nproc)
sudo make install
```

The bindings are importable as the `pystards` package once built and installed.

## Use the header in your own C++ project

StarDS is header-only, so you only need the include path plus any optional
libraries you want to link. Each optional feature is gated by a **compile
definition** (`ENABLE_ZLIB`, `ENABLE_LZ4`, `ENABLE_CURL`, `ENABLE_S3`) — these are
the `-D` preprocessor macros the header checks, and are distinct from the
`STAR_ENABLE_*` **CMake build options** used when building this repo's own targets:

```cmake
# CMakeLists.txt
# In-tree checkout: StarDS/include. Installed: the header is at
# $PREFIX/include/stards/stards.h (use $PREFIX/include/stards, or find_package(STAR)).
include_directories(/path/to/stards/StarDS/include)

find_package(ZLIB)
find_package(CURL)
find_package(OpenSSL)

if(ZLIB_FOUND)
    target_link_libraries(your_target PRIVATE ZLIB::ZLIB)
    target_compile_definitions(your_target PRIVATE ENABLE_ZLIB)
endif()

if(CURL_FOUND)
    target_link_libraries(your_target PRIVATE CURL::libcurl)
    target_compile_definitions(your_target PRIVATE ENABLE_CURL)
endif()

if(OPENSSL_FOUND)
    target_link_libraries(your_target PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(your_target PRIVATE ENABLE_S3)
endif()
```

Then include the single header:

```cpp
#include "stards.h"
using namespace star;

int main() {
    auto store = StarDataset::create("data.stards");
    store->put("matrix", NDArray<double>::zeros({100, 100}));
    store->flush();
    return 0;
}
```

## Next steps

- [Quick Start](quickstart.md) — a 5-minute Python tour.
- [Concepts](concepts.md) — the data model behind `.stards` files.
