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

    auto store = StarDataset::open(vsicurlUrl(srv, "/data.stards"), FileMode::READ_ONLY);

    // Metadata value survives the HTTP round-trip.
    auto label = store->meta.get("label");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->as<std::string>()(0), "vsicurl-test");

    // Array value survives.
    auto arr = store->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 5u);
    for (size_t i = 0; i < 5; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i));

    // The client did a HEAD (to discover Content-Length) at least once.
    bool saw_head = false;
    for (const auto& r : srv.requests()) if (r.method == "HEAD") saw_head = true;
    EXPECT_TRUE(saw_head);
}

TEST_F(VsiCurlTest, MultiChunkRangeReads) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";

    // 40k int64 = 320 KB uncompressed. With NONE compression the array block is
    // large enough that reading it issues multiple 64 KB Range GETs.
    const size_t N = 40000;
    std::string local = tempFile("big");
    {
        StarConfig cfg;
        cfg.compression = CompressionAlgorithm::NONE;  // keep it large -> multi-chunk
        auto store = StarDataset::create(local, cfg);
        NDArray<int64_t> arr({N});
        for (size_t i = 0; i < N; ++i) arr.flat(i) = static_cast<int64_t>(i * 3 + 1);
        store->put("big", std::move(arr));
        store->flush();
    }
    srv.addObject("/big.stards", star_test::read_file_bytes(local));

    auto store = StarDataset::open(vsicurlUrl(srv, "/big.stards"), FileMode::READ_ONLY);
    auto arr = store->get<int64_t>("big");
    ASSERT_EQ(arr.size(), N);
    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(arr.flat(i), static_cast<int64_t>(i * 3 + 1)) << "mismatch at " << i;
    }

    // Multiple ranged GETs should have occurred (fetchRange in 64 KB chunks).
    int ranged_gets = 0;
    for (const auto& r : srv.requests()) {
        if (r.method == "GET" && !r.range.empty()) ranged_gets++;
    }
    EXPECT_GT(ranged_gets, 1) << "expected the large array to span multiple Range reads";
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
    EXPECT_TRUE(store->isReadOnly());
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
        store->saveTo(dst);
    }

    // The saved local copy is complete and readable.
    auto copy = StarDataset::open(dst, FileMode::READ_ONLY);
    auto arr = copy->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 4u);
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i));
    EXPECT_EQ(copy->meta.get("label")->as<std::string>()(0), "vsicurl-test");
}

#endif  // ENABLE_CURL
