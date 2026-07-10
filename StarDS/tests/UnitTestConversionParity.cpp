// Conversion parity test: convert the same JSON example with every supported
// compression codec (none / gzip / lz4 / gzip-shuffle / lz4-shuffle) via the
// real star_translate tool, then verify all outputs decode to byte-identical
// data. This guards the compression + shuffle round-trip end to end.
//
// The star_translate executable path and the example JSON path are injected at
// build time via the STAR_TRANSLATE_EXE / STAR_EXAMPLE_JSON compile definitions
// (see Star/tests/CMakeLists.txt), so the test is independent of the working dir.

#include <gtest/gtest.h>
#include "stards.h"
#include "Fixtures.h"

#include <cstdlib>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace star;

#ifndef STAR_TRANSLATE_EXE
#define STAR_TRANSLATE_EXE ""
#endif
#ifndef STAR_EXAMPLE_JSON
#define STAR_EXAMPLE_JSON ""
#endif

namespace {

// A codec-independent snapshot of one stored value.
struct ValueSnapshot {
    int dtype = -1;                 // DataType as int
    std::vector<size_t> shape;
    std::vector<char> bytes;        // raw element bytes (host order), or UTF-8 for strings
    bool operator==(const ValueSnapshot& o) const {
        return dtype == o.dtype && shape == o.shape && bytes == o.bytes;
    }
};

// Serialize an NDArray<T>'s elements into canonical bytes. Numeric types are raw
// contiguous bytes; strings are length-prefixed so distinct string layouts can't
// alias to equal byte blobs.
template <typename T>
std::vector<char> canonical_bytes(const NDArray<T>& arr) {
    std::vector<char> out;
    if constexpr (std::is_same<T, std::string>::value) {
        for (const auto& s : arr.data()) {
            uint32_t n = static_cast<uint32_t>(s.size());
            const char* np = reinterpret_cast<const char*>(&n);
            out.insert(out.end(), np, np + sizeof(n));
            out.insert(out.end(), s.begin(), s.end());
        }
    } else {
        const auto& d = arr.data();
        const char* p = reinterpret_cast<const char*>(d.data());
        out.assign(p, p + d.size() * sizeof(T));
    }
    return out;
}

// Snapshot a value given its dtype, reading it from the array namespace (via
// store.get<T>) or the metadata namespace (via a MetadataValue).
template <typename Reader>
ValueSnapshot snapshot_by_dtype(DataType dt, Reader&& read) {
    ValueSnapshot snap;
    snap.dtype = static_cast<int>(dt);
    switch (dt) {
        case DataType::INT8:    { auto a = read.template operator()<int8_t>();   snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::INT16:   { auto a = read.template operator()<int16_t>();  snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::INT32:   { auto a = read.template operator()<int32_t>();  snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::INT64:   { auto a = read.template operator()<int64_t>();  snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::UINT8:   { auto a = read.template operator()<uint8_t>();  snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::UINT16:  { auto a = read.template operator()<uint16_t>(); snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::UINT32:  { auto a = read.template operator()<uint32_t>(); snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::UINT64:  { auto a = read.template operator()<uint64_t>(); snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::FLOAT32: { auto a = read.template operator()<float>();    snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::FLOAT64: { auto a = read.template operator()<double>();   snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        case DataType::STRING:  { auto a = read.template operator()<std::string>(); snap.shape = a.shape(); snap.bytes = canonical_bytes(a); break; }
        default: ADD_FAILURE() << "Unsupported dtype " << static_cast<int>(dt);
    }
    return snap;
}

// Read every value in a .stards file (array namespace + metadata namespace) into a
// key -> snapshot map.
std::map<std::string, ValueSnapshot> snapshot_file(const std::string& path) {
    std::map<std::string, ValueSnapshot> out;
    auto store = StarDataset::open(path, FileMode::READ_ONLY);

    // Array-namespace keys: get_all_keys() minus any layer-prefixed internals.
    for (const auto& key : store->get_all_keys()) {
        if (key.rfind("__layer_", 0) == 0) continue;
        DataType dt = store->dtype_of(key);
        auto reader = [&]<typename T>() { return store->get<T>(key); };
        out[key] = snapshot_by_dtype(dt, reader);
    }

    // Metadata-namespace keys.
    for (const auto& key : store->get_metadata_keys()) {
        auto mv = store->meta.get(key);
        EXPECT_NE(mv, nullptr) << "metadata key vanished: " << key;
        if (!mv) continue;
        DataType dt = mv->dtype;
        auto reader = [&]<typename T>() { return mv->template as<T>(); };
        // Use a distinct namespace prefix so an array and metadata key of the
        // same name don't collide in the map.
        out["meta:" + key] = snapshot_by_dtype(dt, reader);
    }
    return out;
}

// Convert STAR_EXAMPLE_JSON to `out_path` with the given codec via star_translate.
// Returns the process exit code.
int run_translate(const std::string& codec, const std::string& out_path) {
    std::string cmd = std::string("\"") + STAR_TRANSLATE_EXE + "\" -c " + codec +
                      " \"" + STAR_EXAMPLE_JSON + "\" \"" + out_path + "\" > /dev/null 2>&1";
    return std::system(cmd.c_str());
}

class ConversionParityTest : public star_test::TempDirTest {
protected:
    void SetUp() override {
        star_test::TempDirTest::SetUp();  // per-test temp dir (auto-deleted)
        if (std::string(STAR_TRANSLATE_EXE).empty() || std::string(STAR_EXAMPLE_JSON).empty()) {
            GTEST_SKIP() << "star_translate/example JSON paths not configured";
        }
        // Confirm the example JSON is present.
        FILE* f = std::fopen(STAR_EXAMPLE_JSON, "rb");
        if (!f) GTEST_SKIP() << "example JSON not found: " << STAR_EXAMPLE_JSON;
        std::fclose(f);
    }
};

}  // namespace

TEST_F(ConversionParityTest, AllCodecsProduceIdenticalData) {
    const std::vector<std::string> codecs = {
        "none", "gzip", "lz4", "gzip-shuffle", "lz4-shuffle"};

    // Convert with the reference codec (no compression) first.
    const std::string ref_path = tempFilePath("star_parity_none.stards");
    ASSERT_EQ(run_translate("none", ref_path), 0) << "conversion failed for codec 'none'";
    auto reference = snapshot_file(ref_path);
    ASSERT_FALSE(reference.empty()) << "reference file decoded to zero keys";

    // Every other codec must decode to exactly the same key set and values.
    for (const auto& codec : codecs) {
        if (codec == "none") continue;
        SCOPED_TRACE("codec=" + codec);

        const std::string path = tempFilePath("star_parity_" + codec + ".stards");
        ASSERT_EQ(run_translate(codec, path), 0) << "conversion failed for codec " << codec;

        auto snap = snapshot_file(path);

        // Same set of keys.
        ASSERT_EQ(snap.size(), reference.size())
            << codec << " produced a different number of keys";

        // Same value for every key (dtype, shape, bytes).
        for (const auto& [key, ref_val] : reference) {
            auto it = snap.find(key);
            ASSERT_NE(it, snap.end()) << codec << " is missing key: " << key;
            EXPECT_EQ(it->second, ref_val)
                << codec << " differs from 'none' for key: " << key
                << " (dtype " << ref_val.dtype << ", shape dims " << ref_val.shape.size() << ")";
        }

        std::remove(path.c_str());
    }
    std::remove(ref_path.c_str());
}
