/**
 * @file UnitTestPrefetch.cpp
 * @brief Unit tests for StarDataset::prefetch() — the batch read-ahead API that
 *        loads several arrays in parallel (serial I/O on the shared reader, but
 *        each array's decompress+unshuffle+materialize dispatched to the pool).
 *
 * The contract prefetch() must uphold: after it returns, the named arrays are
 * cached and subsequent get<T>() calls return data BYTE-IDENTICAL to loading each
 * key serially. These tests pin that equivalence across dtypes and compression
 * modes (incl. the shuffle codecs whose decode is fused), threaded and
 * single-threaded, plus the edge cases (already-cached keys, metadata-block keys,
 * unknown keys, empty request).
 */

#include "stards.h"
#include <gtest/gtest.h>
#include "Fixtures.h"
#include <vector>
#include <string>
#include <random>

using namespace star;

namespace {

// Reset global threading to auto after any test that changed it, so ordering
// between tests can't leak a single-threaded pool into a later case.
struct ThreadGuard {
    ~ThreadGuard() { setNumThreads(0); }
};

constexpr int    kCols = 12;
constexpr size_t kRows = 50000;   // > threading thresholds when compressed in blocks

std::string col(int i) { return "c" + std::to_string(i); }

// Populate a dataset with a deterministic mix of int32/float64/uint8 columns.
void populate(std::shared_ptr<StarDataset>& ds) {
    std::mt19937_64 rng(20260731);
    for (int c = 0; c < kCols; ++c) {
        if (c % 3 == 0) {
            NDArray<int32_t> a({kRows});
            for (auto& v : a.data()) v = static_cast<int32_t>(rng());
            ds->put(col(c), a);
        } else if (c % 3 == 1) {
            NDArray<double> a({kRows});
            for (auto& v : a.data()) v = static_cast<double>(rng() % 1000000) * 0.25;
            ds->put(col(c), a);
        } else {
            NDArray<uint8_t> a({kRows});
            for (auto& v : a.data()) v = static_cast<uint8_t>(rng());
            ds->put(col(c), a);
        }
    }
}

// Assert that a prefetched dataset returns the same bytes as a serial reference,
// for every column.
void expectAllColumnsEqual(StarDataset& ref, StarDataset& pf) {
    for (int c = 0; c < kCols; ++c) {
        if (c % 3 == 0) {
            EXPECT_EQ(ref.get<int32_t>(col(c)).data(), pf.get<int32_t>(col(c)).data())
                << "int32 column " << col(c);
        } else if (c % 3 == 1) {
            EXPECT_EQ(ref.get<double>(col(c)).data(), pf.get<double>(col(c)).data())
                << "float64 column " << col(c);
        } else {
            EXPECT_EQ(ref.get<uint8_t>(col(c)).data(), pf.get<uint8_t>(col(c)).data())
                << "uint8 column " << col(c);
        }
    }
}

}  // namespace

class PrefetchTest : public star_test::TempDirTest {
protected:
    // Build a dataset with the given codec, return the file path.
    std::string build(CompressionAlgorithm codec) {
        std::string path = tempStardsFile("prefetch");
        StarConfig cfg;
        cfg.compression = codec;
        auto ds = StarDataset::create(path, cfg);
        populate(ds);
        ds->flush();
        return path;
    }

    std::vector<std::string> allKeys() const {
        std::vector<std::string> keys;
        for (int c = 0; c < kCols; ++c) keys.push_back(col(c));
        return keys;
    }
};

// Prefetched reads equal serial reads, for a plain (non-shuffle) codec.
TEST_F(PrefetchTest, MatchesSerialGet_LZ4) {
    std::string path = build(CompressionAlgorithm::LZ4);
    auto ref = StarDataset::open(path, FileMode::READ_ONLY);
    auto pf  = StarDataset::open(path, FileMode::READ_ONLY);
    pf->prefetch(allKeys());
    expectAllColumnsEqual(*ref, *pf);
}

// Same, for a shuffle codec — this exercises the fused byte-unshuffle on the
// batch decode path.
TEST_F(PrefetchTest, MatchesSerialGet_LZ4ShuffleBlock) {
    std::string path = build(CompressionAlgorithm::LZ4_SHUFFLE_BLOCK);
    auto ref = StarDataset::open(path, FileMode::READ_ONLY);
    auto pf  = StarDataset::open(path, FileMode::READ_ONLY);
    pf->prefetch(allKeys());
    expectAllColumnsEqual(*ref, *pf);
}

// Uncompressed data goes through the same batch path (no shuffle, plain memcpy).
TEST_F(PrefetchTest, MatchesSerialGet_None) {
    std::string path = build(CompressionAlgorithm::NONE);
    auto ref = StarDataset::open(path, FileMode::READ_ONLY);
    auto pf  = StarDataset::open(path, FileMode::READ_ONLY);
    pf->prefetch(allKeys());
    expectAllColumnsEqual(*ref, *pf);
}

// Single-threaded mode (no pool) must still produce identical results — it
// degrades to loading each key in turn.
TEST_F(PrefetchTest, SingleThreadedFallbackMatches) {
    ThreadGuard guard;
    std::string path = build(CompressionAlgorithm::LZ4_SHUFFLE_BLOCK);
    setNumThreads(1);  // no thread pool created on open
    auto ref = StarDataset::open(path, FileMode::READ_ONLY);
    auto pf  = StarDataset::open(path, FileMode::READ_ONLY);
    pf->prefetch(allKeys());
    expectAllColumnsEqual(*ref, *pf);
}

// Prefetch is idempotent and tolerates a partial pre-load: pre-load one column,
// then prefetch all; the pre-loaded one is skipped and everything still matches.
TEST_F(PrefetchTest, IdempotentAndPartialPreload) {
    std::string path = build(CompressionAlgorithm::LZ4);
    auto ref = StarDataset::open(path, FileMode::READ_ONLY);
    auto pf  = StarDataset::open(path, FileMode::READ_ONLY);

    (void)pf->get<int32_t>(col(0));  // c0 now cached
    pf->prefetch(allKeys());          // must skip c0, load the rest
    pf->prefetch(allKeys());          // all cached now — no-op
    expectAllColumnsEqual(*ref, *pf);
}

// An unknown key throws (same contract as get()), and nothing partial is left in
// a bad state — the known keys can still be read afterward.
TEST_F(PrefetchTest, UnknownKeyThrows) {
    std::string path = build(CompressionAlgorithm::LZ4);
    auto pf = StarDataset::open(path, FileMode::READ_ONLY);
    EXPECT_THROW(pf->prefetch({col(0), "does_not_exist"}), std::runtime_error);
    // The dataset is still usable.
    EXPECT_NO_THROW((void)pf->get<int32_t>(col(0)));
}

// Empty request is a no-op.
TEST_F(PrefetchTest, EmptyRequestIsNoOp) {
    std::string path = build(CompressionAlgorithm::LZ4);
    auto pf = StarDataset::open(path, FileMode::READ_ONLY);
    EXPECT_NO_THROW(pf->prefetch({}));
}

// prefetch operates on the named-array namespace (what get<T>() reads). Values
// written through meta.put() live in the separate metadata registry, so naming
// one in a prefetch is a "key not found" — and doing so must not disturb the
// metadata, which is still readable via meta.get() afterward.
TEST_F(PrefetchTest, MetadataKeysAreNotPrefetchable) {
    std::string path = tempStardsFile("prefetch_meta");
    {
        auto ds = StarDataset::create(path);
        ds->put("big", NDArray<double>::full({kRows}, 3.0));
        ds->meta.put("scale", NDArray<double>({}, 1.25));
        ds->flush();
    }
    auto pf = StarDataset::open(path, FileMode::READ_ONLY);
    // A metadata key is not an array key: it throws like any unknown key.
    EXPECT_THROW(pf->prefetch({"scale"}), std::runtime_error);
    // The array key alone prefetches fine, and metadata still reads back.
    EXPECT_NO_THROW(pf->prefetch({"big"}));
    EXPECT_EQ(pf->get<double>("big").size(), kRows);
    auto scale = pf->meta.get("scale");
    ASSERT_NE(scale, nullptr);
    EXPECT_DOUBLE_EQ(scale->as<double>().data()[0], 1.25);
}
