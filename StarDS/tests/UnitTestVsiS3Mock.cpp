/**
 * @file UnitTestVsiS3Mock.cpp
 * @brief Functional tests for the /vsis3 read/write path against an in-process
 *        mock S3 server, using the AWS_S3_ENDPOINT endpoint override (path-style,
 *        HTTP). No real AWS credentials or network required.
 *
 * The mock is the same localhost HTTP server as the /vsicurl tests; with
 * path-style addressing the S3 object at /vsis3/<bucket>/<key> maps to the HTTP
 * path "/<bucket>/<key>", so the mock serves it directly.
 */
#ifdef ENABLE_S3

// MockHttpServer.h pulls in the POSIX socket headers; include it before stards.h
// (which includes <curl/curl.h>) so the socket types are defined cleanly first.
#include "MockHttpServer.h"

#include <gtest/gtest.h>
#include "stards.h"
#include "Fixtures.h"

#include <ghc/fs_std.hpp>
#include <cstdlib>
#include <random>
#include <sstream>

using namespace star;
using star_test::MockHttpServer;

namespace {
// RAII env-var setter/restorer (mirrors S3Test::EnvGuard).
struct EnvGuard {
    std::string name;
    std::string old_value;
    bool had_value;
    EnvGuard(const std::string& n, const std::string& v) : name(n) {
        const char* cur = std::getenv(n.c_str());
        had_value = cur != nullptr;
        if (had_value) old_value = cur;
        setenv(n.c_str(), v.c_str(), 1);
    }
    ~EnvGuard() {
        if (had_value) setenv(name.c_str(), old_value.c_str(), 1);
        else unsetenv(name.c_str());
    }
};
}  // namespace

// Each test gets its own temp directory (auto-deleted); tempFile() returns a
// unique path inside it (parallel-safe).
class VsiS3MockTest : public star_test::TempDirTest {
protected:
    std::string tempFile(const std::string& prefix = "s3mock") {
        return tempStardsFile(prefix);
    }

    // Environment that points the S3 code at the local mock: path-style, HTTP,
    // dummy credentials, fixed region.
    struct S3MockEnv {
        EnvGuard endpoint, vhost, https, key, secret, region;
        explicit S3MockEnv(int port)
            : endpoint("AWS_S3_ENDPOINT", "127.0.0.1:" + std::to_string(port)),
              vhost("AWS_VIRTUAL_HOSTING", "FALSE"),
              https("AWS_HTTPS", "NO"),
              key("AWS_ACCESS_KEY_ID", "AKIATESTTESTTESTTEST"),
              secret("AWS_SECRET_ACCESS_KEY", "testsecrettestsecrettestsecrettestsecre"),
              region("AWS_DEFAULT_REGION", "us-east-1") {}
    };
};

TEST_F(VsiS3MockTest, ReadRoundTrip) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";
    S3MockEnv env(srv.port());

    // Build a real .stards and register it under the path-style key.
    std::string local = tempFile("src");
    {
        auto store = StarDataset::create(local);
        store->meta.put("label", NDArray<std::string>({}, "s3-mock"));
        NDArray<int64_t> arr({6});
        for (size_t i = 0; i < 6; ++i) arr.flat(i) = static_cast<int64_t>(i * 2);
        store->put("data", std::move(arr));
        store->flush();
    }
    srv.addObject("/test-bucket/obj.stards", star_test::read_file_bytes(local));

    auto store = StarDataset::open("/vsis3/test-bucket/obj.stards", FileMode::READ_ONLY);
    EXPECT_EQ(store->meta.get("label")->as<std::string>()(0), "s3-mock");
    auto arr = store->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 6u);
    for (size_t i = 0; i < 6; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i * 2));

    // Requests carried a SigV4 Authorization header and a Range read happened.
    bool saw_auth = false, saw_range = false;
    for (const auto& r : srv.requests()) {
        if (r.had_authorization) saw_auth = true;
        if (r.method == "GET" && !r.range.empty()) saw_range = true;
    }
    EXPECT_TRUE(saw_auth) << "expected a signed Authorization header";
    EXPECT_TRUE(saw_range) << "expected a ranged GET for the array block";
}

TEST_F(VsiS3MockTest, ReadRoundTripS3Uri) {
    // An "s3://bucket/key" URI must resolve identically to "/vsis3/bucket/key"
    // — GDAL-compatibility shorthand.
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";
    S3MockEnv env(srv.port());

    std::string local = tempFile("src");
    {
        auto store = StarDataset::create(local);
        store->meta.put("label", NDArray<std::string>({}, "s3-uri"));
        NDArray<int64_t> arr({6});
        for (size_t i = 0; i < 6; ++i) arr.flat(i) = static_cast<int64_t>(i * 3);
        store->put("data", std::move(arr));
        store->flush();
    }
    srv.addObject("/test-bucket/uri.stards", star_test::read_file_bytes(local));

    auto store = StarDataset::open("s3://test-bucket/uri.stards", FileMode::READ_ONLY);
    EXPECT_EQ(store->meta.get("label")->as<std::string>()(0), "s3-uri");
    auto arr = store->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 6u);
    for (size_t i = 0; i < 6; ++i) EXPECT_EQ(arr.flat(i), static_cast<int64_t>(i * 3));

    bool saw_auth = false;
    for (const auto& r : srv.requests()) if (r.had_authorization) saw_auth = true;
    EXPECT_TRUE(saw_auth) << "expected a signed Authorization header via s3:// path";
}

TEST_F(VsiS3MockTest, WriteRoundTrip) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";
    S3MockEnv env(srv.port());

    const std::string s3path = "/vsis3/test-bucket/written.stards";
    {
        auto store = StarDataset::create(s3path);
        store->meta.put("who", NDArray<std::string>({}, "writer"));
        NDArray<int64_t> arr({4});
        for (size_t i = 0; i < 4; ++i) arr.flat(i) = static_cast<int64_t>(100 + i);
        store->put("nums", std::move(arr));
        store->flush();  // -> PUT to the mock
    }

    // The mock captured the uploaded object under the path-style path.
    ASSERT_TRUE(srv.hasObject("/test-bucket/written.stards"));

    // A PUT with a signed Authorization header was issued.
    bool saw_put_auth = false;
    for (const auto& r : srv.requests()) {
        if (r.method == "PUT" && r.had_authorization) saw_put_auth = true;
    }
    EXPECT_TRUE(saw_put_auth);

    // Read it back through the S3 path and verify contents.
    auto store = StarDataset::open(s3path, FileMode::READ_ONLY);
    EXPECT_EQ(store->meta.get("who")->as<std::string>()(0), "writer");
    auto nums = store->get<int64_t>("nums");
    ASSERT_EQ(nums.size(), 4u);
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(nums.flat(i), static_cast<int64_t>(100 + i));
}

TEST_F(VsiS3MockTest, RegionRedirectHandled) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";
    S3MockEnv env(srv.port());

    std::string local = tempFile("src");
    {
        auto store = StarDataset::create(local);
        store->put("data", NDArray<int64_t>({3}, 7));
        store->flush();
    }
    srv.addObject("/test-bucket/redir.stards", star_test::read_file_bytes(local));
    // The first request (a ranged GET, since we no longer HEAD) gets a 301 +
    // region; the S3RangeReader re-signs for the correct region and retries.
    srv.addRegionRedirectOnce("/test-bucket/redir.stards", "us-west-2");

    auto store = StarDataset::open("/vsis3/test-bucket/redir.stards", FileMode::READ_ONLY);
    auto arr = store->get<int64_t>("data");
    ASSERT_EQ(arr.size(), 3u);
    EXPECT_EQ(arr.flat(0), 7);

    // The 301 must have been followed by a successful retry. We no longer issue
    // HEADs at all; the redirect is absorbed on the GET path, so assert that a
    // GET succeeded (data read back) and that NO HEAD was ever sent.
    int heads = 0, gets = 0;
    for (const auto& r : srv.requests()) {
        if (r.method == "HEAD") heads++;
        if (r.method == "GET") gets++;
    }
    EXPECT_EQ(heads, 0) << "the optimized read path should never issue a HEAD";
    EXPECT_GE(gets, 2) << "expected a retry GET after the 301 region redirect";
}

TEST_F(VsiS3MockTest, MissingObjectThrows) {
    MockHttpServer srv;
    if (!srv.ok()) GTEST_SKIP() << "could not bind a local port for the mock server";
    S3MockEnv env(srv.port());

    EXPECT_THROW(
        StarDataset::open("/vsis3/test-bucket/absent.stards", FileMode::READ_ONLY),
        std::runtime_error);
}

#endif  // ENABLE_S3
