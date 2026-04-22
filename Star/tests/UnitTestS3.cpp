/**
 * @file UnitTestS3.cpp
 * @brief Unit tests for S3 support
 */

#ifdef ENABLE_S3

#include <gtest/gtest.h>
#include "star.h"
#include <iostream>
#include <fstream>
#include <ghc/fs_std.hpp>
#include <random>


using namespace star;

//==============================================================================
// Test Fixture for S3 Tests
//==============================================================================

class S3Test : public ::testing::Test {
protected:
    std::vector<std::string> temp_files;

    void SetUp() override {
        // Nothing to do in setup
    }

    void TearDown() override {
        // Clean up all temporary files created during the test
        for (const auto& file : temp_files) {
            if (fs::exists(file)) {
                try {
                    fs::remove(file);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to delete temp file " << file
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }

    std::string createTempFile(const std::string& prefix = "test_s3") {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;

        std::stringstream ss;
        ss << "/tmp/" << prefix << "_" << std::hex << dis(gen) << ".star";
        std::string filename = ss.str();

        temp_files.push_back(filename);
        return filename;
    }

    // Save and restore environment variables
    std::string getEnv(const char* name) {
        const char* val = std::getenv(name);
        return val ? std::string(val) : "";
    }

    void setEnv(const char* name, const std::string& value) {
        if (value.empty()) {
            unsetenv(name);
        } else {
            setenv(name, value.c_str(), 1);
        }
    }

    struct EnvGuard {
        std::string name;
        std::string old_value;
        bool had_value;

        EnvGuard(const std::string& n, const std::string& new_val, S3Test* test)
            : name(n), had_value(false) {
            const char* val = std::getenv(n.c_str());
            if (val) {
                old_value = val;
                had_value = true;
            }
            test->setEnv(n.c_str(), new_val);
        }

        ~EnvGuard() {
            if (had_value) {
                setenv(name.c_str(), old_value.c_str(), 1);
            } else {
                unsetenv(name.c_str());
            }
        }
    };
};

//==============================================================================
// Path Parsing Tests
//==============================================================================

TEST_F(S3Test, PathParsingS3) {
    auto info = parseFilePath("/vsis3/my-bucket/path/to/object.star");
    EXPECT_EQ(info.type, FilePathInfo::S3);
    EXPECT_EQ(info.bucket, "my-bucket");
    EXPECT_EQ(info.key, "path/to/object.star");
    EXPECT_FALSE(info.region.empty());
}

TEST_F(S3Test, PathParsingHTTP) {
    auto info = parseFilePath("/vsicurl/https://example.com/file.star");
    EXPECT_EQ(info.type, FilePathInfo::HTTP);
    EXPECT_EQ(info.path, "https://example.com/file.star");
}

TEST_F(S3Test, PathParsingLocal) {
    auto info = parseFilePath("/tmp/local_file.star");
    EXPECT_EQ(info.type, FilePathInfo::LOCAL);
    EXPECT_EQ(info.path, "/tmp/local_file.star");
}

TEST_F(S3Test, PathParsingInvalidS3) {
    // Missing key after bucket
    EXPECT_THROW(parseFilePath("/vsis3/bucket-only"), std::runtime_error);
}

//==============================================================================
// FileMode Tests
//==============================================================================

TEST_F(S3Test, FileModesAvailable) {
    // Just verify that FileMode enum is available
    FileMode rw = FileMode::READ_WRITE;
    FileMode ro = FileMode::READ_ONLY;
    EXPECT_NE(rw, ro);
}

//==============================================================================
// Credential Resolution Tests
//==============================================================================

TEST_F(S3Test, CredentialResolutionFromEnvironment) {
    // Set test credentials via environment
    EnvGuard key("AWS_ACCESS_KEY_ID", "TEST_ACCESS_KEY", this);
    EnvGuard secret("AWS_SECRET_ACCESS_KEY", "TEST_SECRET_KEY", this);

    auto creds = S3Credentials::resolve();
    EXPECT_EQ(creds.access_key, "TEST_ACCESS_KEY");
    EXPECT_EQ(creds.secret_key, "TEST_SECRET_KEY");
}

TEST_F(S3Test, CredentialResolutionWithSessionToken) {
    // Set test credentials with session token
    EnvGuard key("AWS_ACCESS_KEY_ID", "TEST_ACCESS_KEY", this);
    EnvGuard secret("AWS_SECRET_ACCESS_KEY", "TEST_SECRET_KEY", this);
    EnvGuard token("AWS_SESSION_TOKEN", "TEST_SESSION_TOKEN", this);

    auto creds = S3Credentials::resolve();
    EXPECT_EQ(creds.access_key, "TEST_ACCESS_KEY");
    EXPECT_EQ(creds.secret_key, "TEST_SECRET_KEY");
    EXPECT_EQ(creds.session_token, "TEST_SESSION_TOKEN");
}

TEST_F(S3Test, CredentialResolutionPriority) {
    // Environment variables should take priority over credentials file
    EnvGuard key("AWS_ACCESS_KEY_ID", "ENV_KEY", this);
    EnvGuard secret("AWS_SECRET_ACCESS_KEY", "ENV_SECRET", this);

    auto creds = S3Credentials::resolve();
    EXPECT_EQ(creds.access_key, "ENV_KEY");
    EXPECT_EQ(creds.secret_key, "ENV_SECRET");
}

//==============================================================================
// Config File Parsing Tests
//==============================================================================

TEST_F(S3Test, ConfigFileParsingBasic) {
    std::string config_path = createTempFile("aws_config");

    // Write test config
    {
        std::ofstream config(config_path);
        config << "[default]\n";
        config << "region = us-west-2\n";
        config << "output = json\n";
        config << "\n";
        config << "[profile test-profile]\n";
        config << "region = eu-central-1\n";
        config << "output = text\n";
    }

    AWSConfigParser parser(config_path);

    EXPECT_TRUE(parser.hasProfile("default"));
    EXPECT_TRUE(parser.hasProfile("test-profile"));  // Parser strips "profile " prefix
    EXPECT_FALSE(parser.hasProfile("nonexistent"));

    EXPECT_EQ(parser.getValue("default", "region"), "us-west-2");
    EXPECT_EQ(parser.getValue("default", "output"), "json");
    EXPECT_EQ(parser.getValue("test-profile", "region"), "eu-central-1");  // Use stripped name
}

TEST_F(S3Test, ConfigFileParsingWithSpaces) {
    std::string config_path = createTempFile("aws_config_spaces");

    // Write test config with various spacing
    {
        std::ofstream config(config_path);
        config << "[default]\n";
        config << "region = us-west-2  \n";  // Trailing spaces
        config << "  output=json\n";  // Leading spaces, no space around =
        config << "key = value with spaces\n";
    }

    AWSConfigParser parser(config_path);

    EXPECT_EQ(parser.getValue("default", "region"), "us-west-2");
    EXPECT_EQ(parser.getValue("default", "output"), "json");
    EXPECT_EQ(parser.getValue("default", "key"), "value with spaces");
}

//==============================================================================
// Read-Only Mode Tests
//==============================================================================

TEST_F(S3Test, ReadOnlyMode) {
    std::string testFile = createTempFile("readonly");

    // Create test file
    {
        auto store = StarDataset::create(testFile);
        store->meta.put("data", NDArray<int64_t>({}, 42));
        store->flush();
    }

    // Open in read-only mode
    auto store = StarDataset::open(testFile, FileMode::READ_ONLY);

    // Verify can read
    auto data = store->meta.get("data");
    ASSERT_NE(data, nullptr);
    auto arr = data->as<int64_t>();
    EXPECT_EQ(arr(0), 42);

    // Verify can modify in memory
    store->meta.put("new_data", NDArray<int64_t>({}, 100));

    // Verify cannot flush
    EXPECT_THROW(store->flush(), std::runtime_error);

    // Verify isReadOnly()
    EXPECT_TRUE(store->isReadOnly());
}

TEST_F(S3Test, ReadWriteModeDefault) {
    std::string testFile = createTempFile("readwrite");

    // Default should be read-write
    auto store = StarDataset::create(testFile);
    EXPECT_FALSE(store->isReadOnly());

    store->meta.put("data", NDArray<int64_t>({}, 123));
    EXPECT_NO_THROW(store->flush());
}

//==============================================================================
// SaveTo Tests (Local Files)
//==============================================================================

TEST_F(S3Test, SaveToLocal) {
    std::string sourceFile = createTempFile("saveto_source");
    std::string targetFile = createTempFile("saveto_target");

    // Create source file
    {
        auto store = StarDataset::create(sourceFile);
        store->meta.put("original", NDArray<int64_t>({}, 1));
        store->flush();
    }

    // Open, modify, and save to different file
    auto store = StarDataset::open(sourceFile);
    store->meta.put("modified", NDArray<int64_t>({}, 2));
    store->saveTo(targetFile);

    // Verify both files exist
    EXPECT_TRUE(fs::exists(sourceFile));
    EXPECT_TRUE(fs::exists(targetFile));

    // Verify target has both values
    auto target = StarDataset::open(targetFile);
    EXPECT_TRUE(target->meta.contains("original"));
    EXPECT_TRUE(target->meta.contains("modified"));

    auto orig = target->meta.get("original");
    ASSERT_NE(orig, nullptr);
    EXPECT_EQ(orig->as<int64_t>()(0), 1);

    auto mod = target->meta.get("modified");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->as<int64_t>()(0), 2);
}

TEST_F(S3Test, ReadOnlySaveToAnotherFile) {
    std::string sourceFile = createTempFile("readonly_source");
    std::string targetFile = createTempFile("readonly_target");

    // Create source
    {
        auto store = StarDataset::create(sourceFile);
        store->meta.put("data", NDArray<int64_t>({}, 99));
        store->flush();
    }

    // Open read-only
    auto store = StarDataset::open(sourceFile, FileMode::READ_ONLY);

    // Modify in memory
    store->meta.put("modified", NDArray<int64_t>({}, 88));

    // Cannot save to source
    EXPECT_THROW(store->saveTo(sourceFile), std::runtime_error);

    // Can save to different file
    EXPECT_NO_THROW(store->saveTo(targetFile));

    // Verify source unchanged
    auto source_check = StarDataset::open(sourceFile);
    EXPECT_TRUE(source_check->meta.contains("data"));
    EXPECT_FALSE(source_check->meta.contains("modified"));

    // Verify target has both
    auto target_check = StarDataset::open(targetFile);
    EXPECT_TRUE(target_check->meta.contains("data"));
    EXPECT_TRUE(target_check->meta.contains("modified"));
}

TEST_F(S3Test, GetFilename) {
    std::string testFile = createTempFile("filename_test");

    auto store = StarDataset::create(testFile);
    EXPECT_EQ(store->getFilename(), testFile);

    store->meta.put("data", NDArray<int64_t>({}, 42));
    store->flush();

    // Filename should not change after flush
    EXPECT_EQ(store->getFilename(), testFile);
}

//==============================================================================
// S3 Integration Tests (require TEST_S3_BUCKET environment variable)
//==============================================================================

TEST_F(S3Test, S3IntegrationWriteRead) {
    const char* test_bucket = std::getenv("TEST_S3_BUCKET");
    if (!test_bucket) {
        GTEST_SKIP() << "TEST_S3_BUCKET not set - skipping S3 integration test";
    }

    // Generate unique key
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::stringstream ss;
    ss << "test_" << std::hex << dis(gen) << ".star";
    std::string s3_path = std::string("/vsis3/") + test_bucket + "/" + ss.str();

    // Write to S3
    {
        auto store = StarDataset::create(s3_path);
        store->meta.put("test_data", NDArray<int64_t>({}, 12345));
        store->meta.put("test_string", NDArray<std::string>({}, "hello s3"));
        EXPECT_NO_THROW(store->flush());
    }

    // Read from S3
    {
        auto store = StarDataset::create(s3_path);
        EXPECT_TRUE(store->meta.contains("test_data"));
        EXPECT_TRUE(store->meta.contains("test_string"));

        auto data = store->meta.get("test_data");
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(data->as<int64_t>()(0), 12345);

        auto str = store->meta.get("test_string");
        ASSERT_NE(str, nullptr);
        EXPECT_EQ(str->as<std::string>()(0), "hello s3");
    }

    // TODO: Clean up S3 object after test
    // (Would need S3 DELETE support)
}

TEST_F(S3Test, S3ToLocalSaveTo) {
    const char* test_bucket = std::getenv("TEST_S3_BUCKET");
    if (!test_bucket) {
        GTEST_SKIP() << "TEST_S3_BUCKET not set - skipping S3 integration test";
    }

    // Generate unique key
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::stringstream ss;
    ss << "test_saveto_" << std::hex << dis(gen) << ".star";
    std::string s3_path = std::string("/vsis3/") + test_bucket + "/" + ss.str();
    std::string local_path = createTempFile("s3_to_local");

    // Create S3 file
    {
        auto store = StarDataset::create(s3_path);
        store->meta.put("s3_data", NDArray<double>({10}));
        store->flush();
    }

    // Read from S3, save to local
    {
        auto store = StarDataset::open(s3_path, FileMode::READ_ONLY);
        store->saveTo(local_path);
    }

    // Verify local file
    auto local = StarDataset::create(local_path);
    EXPECT_TRUE(local->meta.contains("s3_data"));
}

TEST_F(S3Test, LocalToS3SaveTo) {
    const char* test_bucket = std::getenv("TEST_S3_BUCKET");
    if (!test_bucket) {
        GTEST_SKIP() << "TEST_S3_BUCKET not set - skipping S3 integration test";
    }

    std::string local_path = createTempFile("local_to_s3");

    // Generate unique key
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::stringstream ss;
    ss << "test_upload_" << std::hex << dis(gen) << ".star";
    std::string s3_path = std::string("/vsis3/") + test_bucket + "/" + ss.str();

    // Create local file
    {
        auto store = StarDataset::create(local_path);
        store->meta.put("local_data", NDArray<int64_t>({}, 777));
        store->flush();
    }

    // Save to S3
    {
        auto store = StarDataset::open(local_path);
        store->saveTo(s3_path);
    }

    // Verify S3 file
    auto s3_store = StarDataset::create(s3_path);
    EXPECT_TRUE(s3_store->meta.contains("local_data"));
    auto data = s3_store->meta.get("local_data");
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->as<int64_t>()(0), 777);
}

//==============================================================================
// AWS Signature V4 Tests
//==============================================================================

TEST_F(S3Test, AWSV4SignerBasic) {
    // Test with AWS example credentials from documentation
    AWSV4Signer signer(
        "AKIAIOSFODNN7EXAMPLE",
        "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
        "us-east-1"
    );

    std::map<std::string, std::string> headers;
    headers["host"] = "examplebucket.s3.us-east-1.amazonaws.com";
    headers["x-amz-date"] = "20230101T120000Z";
    headers["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";  // SHA256 of empty string

    std::string auth = signer.signRequest("GET", "examplebucket", "test.txt",
                                         "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                                         headers);

    // Should start with AWS4-HMAC-SHA256
    EXPECT_EQ(auth.substr(0, 16), "AWS4-HMAC-SHA256");

    // Should contain Credential
    EXPECT_NE(auth.find("Credential="), std::string::npos);

    // Should contain SignedHeaders
    EXPECT_NE(auth.find("SignedHeaders="), std::string::npos);

    // Should contain Signature
    EXPECT_NE(auth.find("Signature="), std::string::npos);
}

TEST_F(S3Test, AWSV4SignerWithSessionToken) {
    AWSV4Signer signer(
        "AKIAIOSFODNN7EXAMPLE",
        "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
        "us-east-1",
        "test-session-token"
    );

    std::map<std::string, std::string> headers;
    headers["host"] = "bucket.s3.us-east-1.amazonaws.com";
    headers["x-amz-date"] = "20230101T120000Z";
    headers["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    headers["x-amz-security-token"] = "test-session-token";

    std::string auth = signer.signRequest("PUT", "bucket", "object.txt",
                                         "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                                         headers);

    EXPECT_EQ(auth.substr(0, 16), "AWS4-HMAC-SHA256");
}

#endif  // ENABLE_S3
