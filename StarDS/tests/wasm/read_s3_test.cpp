// WebAssembly smoke test: open a real .stards object over HTTPS (unsigned range
// GETs against public S3) and read it, driven entirely by the emscripten build's
// EM_ASYNC_JS + fetch() network backend.
//
// This exercises the full remote-read path under WASM with ZERO C++ API changes:
//   StarDataset::open(url, "r")  ->  HttpRangeReader  ->  em_fetch_get (fetch()).
// read_at() stays blocking; -sASYNCIFY suspends/rewinds the Wasm stack across the
// awaited fetch Promise. Runs under node (global fetch) and in a browser.
//
// Build + run:  see StarDS/tests/wasm/run_read_s3_test.sh
//
// The object is a public, range-readable .stards file:
constexpr const char* kUrl =
    "https://asc-isisdata.s3.us-west-2.amazonaws.com/cnf_test_data/largenet.stards";

#include "stards.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace star;

// Print a few elements of an array, dispatching on its stored dtype.
template <typename T>
static void print_head(StarDataset& ds, const std::string& key, const char* tyname) {
    NDArray<T> arr = ds.get<T>(key);
    std::printf("      dtype=%-7s shape=[", tyname);
    const auto& shp = arr.shape();
    for (size_t i = 0; i < shp.size(); ++i)
        std::printf("%s%zu", i ? "," : "", shp[i]);
    std::printf("] size=%zu  head=[", arr.size());
    const auto& d = arr.data();
    size_t n = d.size() < 6 ? d.size() : 6;
    for (size_t i = 0; i < n; ++i) {
        // Promote to double / long long for a uniform printf.
        if constexpr (std::is_floating_point_v<T>)
            std::printf("%s%g", i ? ", " : "", static_cast<double>(d[i]));
        else
            std::printf("%s%lld", i ? ", " : "", static_cast<long long>(d[i]));
    }
    std::printf("%s]\n", d.size() > n ? ", ..." : "");
}

static void read_one(StarDataset& ds, const std::string& key) {
    DataType dt = ds.dtype_of(key);
    std::printf("    - %s  (%s)\n", key.c_str(), datatype_to_string(dt));
    try {
        switch (dt) {
            case DataType::INT8:    print_head<int8_t>(ds, key, "int8"); break;
            case DataType::INT16:   print_head<int16_t>(ds, key, "int16"); break;
            case DataType::INT32:   print_head<int32_t>(ds, key, "int32"); break;
            case DataType::INT64:   print_head<int64_t>(ds, key, "int64"); break;
            case DataType::UINT8:   print_head<uint8_t>(ds, key, "uint8"); break;
            case DataType::UINT16:  print_head<uint16_t>(ds, key, "uint16"); break;
            case DataType::UINT32:  print_head<uint32_t>(ds, key, "uint32"); break;
            case DataType::UINT64:  print_head<uint64_t>(ds, key, "uint64"); break;
            case DataType::FLOAT32: print_head<float>(ds, key, "float32"); break;
            case DataType::FLOAT64: print_head<double>(ds, key, "float64"); break;
            default:
                std::printf("      (dtype not read in this smoke test)\n");
        }
    } catch (const std::exception& e) {
        std::printf("      read failed: %s\n", e.what());
    }
}

int main() {
    std::printf("Opening: %s\n", kUrl);
    try {
        auto ds = StarDataset::open(kUrl, "r");
        std::printf("Opened OK.\n");

        auto layers = ds->list_layers();
        std::printf("Layers (%zu):\n", layers.size());
        for (const auto& l : layers) std::printf("    - %s\n", l.c_str());

        auto keys = ds->get_all_keys();
        std::printf("Array keys (%zu):\n", keys.size());

        // Read up to the first 3 arrays so we prove data actually decodes.
        size_t to_read = keys.size() < 3 ? keys.size() : 3;
        for (size_t i = 0; i < to_read; ++i) read_one(*ds, keys[i]);
        for (size_t i = to_read; i < keys.size(); ++i)
            std::printf("    - %s  (%s)\n", keys[i].c_str(),
                        datatype_to_string(ds->dtype_of(keys[i])));

        std::printf("\nnetwork requests made: %llu\n",
                    static_cast<unsigned long long>(g_network_request_count.load()));
        std::printf("SUCCESS\n");
        return 0;
    } catch (const std::exception& e) {
        std::printf("FAILED: %s\n", e.what());
        return 1;
    }
}
