/**
 * @file UnitTestVsiCurl.cpp
 * @brief Functional tests for the /vsicurl (HTTP) read path, exercising the real
 *        HttpStreamBuf against an in-process mock HTTP server (no network).
 *
 * Pattern (GDAL-style): write a real .stards locally, serve its bytes over a
 * localhost HTTP server, then open it via /vsicurl/http://127.0.0.1:PORT/... and
 * verify both the data read back and the client's HTTP behavior (HEAD probe,
 * Range reads).
 */
#ifdef ENABLE_CURL

// MockHttpServer.h pulls in the POSIX socket headers; include it before stards.h
// (which includes <curl/curl.h>) so the socket types are defined cleanly first.
#include "MockHttpServer.h"

#include <gtest/gtest.h>
#include "stards.h"
#include "Fixtures.h"

#include <ghc/fs_std.hpp>
#include <random>
#include <sstream>

using namespace star;
using star_test::MockHttpServer;

// Each test gets its own temp directory (auto-deleted); tempFile() returns a
// unique path inside it (parallel-safe).
class VsiCurlTest : public star_test::TempDirTest {
protected:
    std::string tempFile(const std::string& prefix = "vsicurl") {
        return tempStardsFile(prefix);
    }

    // Build a .stards locally with one metadata value and one array, return its path.
    std::string makeDataset(size_t array_len) {
        std::string path = tempFile("src");
        auto store = StarDataset::create(path);
        store->meta.put("label", NDArray<std::string>({}, "vsicurl-test"));
        NDArray<int64_t> arr({array_len});
        for (size_t i = 0; i < array_len; ++i) arr.flat(i) = static_cast<int64_t>(i);
        store->put("data", std::move(arr));
        store->flush();
        return path;
    }

    std::string vsicurlUrl(const MockHttpServer& srv, const std::string& objpath) {
        return "/vsicurl/http://127.0.0.1:" + std::to_string(srv.port()) + objpath;
    }
};

TEST_F(VsiCurlTest, ReadRoundTrip) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    std::string local = makeDataset(5);
    srv.addObject("/data.stards", star_test::read_file_bytes(local));

    // Disable the whole-file prefetch so we can observe the individual ranged
    // reads the read path issues (prefetch is exercised separately below).
    OpenOptions opts;
    opts.prefetch_whole_below_bytes = 0;
    auto store = StarDataset::open(vsicurlUrl(srv, "/data.stards"), FileMode::READ_ONLY, opts);

    // Metadata value survives the HTTP round-trip.
    auto label = store->meta.get("label");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->as<std::string>()(0), "vsicurl-test");

    // Array value survives.
    auto arr = store->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 5u);
    for (size_t i = 0; i < 5; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i));

    // The optimized read path issues ranged GETs on one reused connection and
    // NEVER a HEAD (byte ranges come straight from the header index).
    int heads = 0, gets = 0;
    for (const auto& r : srv.requests()) {
        if (r.method == "HEAD") heads++;
        if (r.method == "GET") gets++;
    }
    EXPECT_EQ(heads, 0) << "read path must not issue any HEAD requests";
    EXPECT_GT(gets, 0) << "expected at least one ranged GET";
}

TEST_F(VsiCurlTest, WholeFilePrefetchServesFromCache) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    std::string local = makeDataset(64);
    srv.addObject("/small.stards", star_test::read_file_bytes(local));

    // Default OpenOptions -> small file is prefetched whole on open. Every read
    // is then served from memory, so the total request count is tiny (and there
    // are no HEADs).
    resetNetworkRequestCount();
    auto store = StarDataset::open(vsicurlUrl(srv, "/small.stards"), FileMode::READ_ONLY);
    auto label = store->meta.get("label");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->as<std::string>()(0), "vsicurl-test");
    auto arr = store->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 64u);
    for (size_t i = 0; i < 64; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i));

    int heads = 0;
    for (const auto& r : srv.requests()) if (r.method == "HEAD") heads++;
    EXPECT_EQ(heads, 0) << "prefetch path must not HEAD";
    // Whole-file prefetch = a single GET; reads after that hit the cache.
    EXPECT_LE(getNetworkRequestCount(), 2u)
        << "small-file open+read should be ~1 request (whole-file prefetch)";
}

TEST_F(VsiCurlTest, LargeArrayCoalescedRead) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    // 40k int64 = 320 KB uncompressed. Previously this spanned many 64 KB Range
    // GETs; the coalesced read path now fetches the contiguous block in ONE GET.
    const size_t N = 40000;
    std::string local = tempFile("big");
    {
        StarConfig cfg;
        cfg.compression = CompressionAlgorithm::NONE;
        auto store = StarDataset::create(local, cfg);
        NDArray<int64_t> arr({N});
        for (size_t i = 0; i < N; ++i) arr.flat(i) = static_cast<int64_t>(i * 3 + 1);
        store->put("big", std::move(arr));
        store->flush();
    }
    srv.addObject("/big.stards", star_test::read_file_bytes(local));

    // Disable whole-file prefetch (file is > threshold anyway) to isolate the
    // array read into its own ranged GET(s).
    OpenOptions opts;
    opts.prefetch_whole_below_bytes = 0;
    auto store = StarDataset::open(vsicurlUrl(srv, "/big.stards"), FileMode::READ_ONLY, opts);

    size_t before = getNetworkRequestCount();
    auto arr = store->get<int64_t>("big");
    ASSERT_EQ(arr.size(), N);
    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(arr.flat(i), static_cast<int64_t>(i * 3 + 1)) << "mismatch at " << i;
    }

    // The contiguous array block is fetched in a single request now, not many.
    size_t gets_for_array = getNetworkRequestCount() - before;
    EXPECT_EQ(gets_for_array, 1u)
        << "a contiguous array should be read in exactly one coalesced GET";

    int heads = 0;
    for (const auto& r : srv.requests()) if (r.method == "HEAD") heads++;
    EXPECT_EQ(heads, 0) << "read path must not issue any HEAD requests";
}

TEST_F(VsiCurlTest, MissingUrlThrows) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    // No object registered -> server 404s -> open(read-only) must throw.
    EXPECT_THROW(
        StarDataset::open(vsicurlUrl(srv, "/nope.stards"), FileMode::READ_ONLY),
        std::runtime_error);
}

TEST_F(VsiCurlTest, ReadOnlyCannotFlush) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    std::string local = makeDataset(3);
    srv.addObject("/ro.stards", star_test::read_file_bytes(local));

    auto store = StarDataset::open(vsicurlUrl(srv, "/ro.stards"), "r");
    EXPECT_TRUE(store->is_read_only());
    store->meta.put("added", NDArray<int64_t>({}, 1));  // in-memory only
    EXPECT_THROW(store->flush(), std::runtime_error);
}

TEST_F(VsiCurlTest, SaveToLocalFromHttp) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    std::string local = makeDataset(4);
    srv.addObject("/copy.stards", star_test::read_file_bytes(local));

    std::string dst = tempFile("dst");
    {
        auto store = StarDataset::open(vsicurlUrl(srv, "/copy.stards"), "r");
        store->save_to(dst);
    }

    // The saved local copy is complete and readable.
    auto copy = StarDataset::open(dst, FileMode::READ_ONLY);
    auto arr = copy->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 4u);
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i));
    EXPECT_EQ(copy->meta.get("label")->as<std::string>()(0), "vsicurl-test");
}

#endif  // ENABLE_CURL
