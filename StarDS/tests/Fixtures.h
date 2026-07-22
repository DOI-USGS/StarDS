#pragma once


#include "gtest/gtest.h"
#include <ghc/fs_std.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#ifdef _WIN32
#include <process.h>   // _getpid
#include <cstdlib>     // _putenv_s
#define STARDS_GETPID _getpid
#else
#include <unistd.h>    // getpid
#define STARDS_GETPID getpid
#endif


using namespace std;


class TempTestingFiles : public ::testing::Environment {
  protected:
    fs::path tempDir;

    void SetUp() override;
    void TearDown() override;
};


/**
 * @brief Reusable per-test fixture: each test gets its OWN unique temp directory,
 *        which is recursively deleted in TearDown().
 *
 * This makes tests safe to run in parallel (e.g. `ctest -j`, where each test is a
 * separate process, and multiple TEST_F cases within one process): no two tests
 * ever share a path, and formerly-hardcoded names like "test_blocks.stards" become
 * unique because they live under a per-test directory.
 *
 * Usage:
 *   class MyTest : public star_test::TempDirTest {};
 *   TEST_F(MyTest, Foo) {
 *       std::string path = tempFilePath("data.stards");   // <unique-dir>/data.stards
 *       auto store = StarDataset::create(path);
 *       ...
 *   }
 * The directory (and everything in it) is removed automatically after the test.
 */
namespace star_test {

// Cross-platform environment-variable helpers. On POSIX these expand to exactly
// setenv(name,value,1) / unsetenv(name) (unchanged behavior); on Windows they use
// _putenv_s (MSVC has no setenv/unsetenv). Note: on Windows unsetEnvVar leaves the
// name defined-but-empty (getenv -> ""), which the AWS code treats as "absent".
inline int setEnvVar(const char* name, const std::string& value) {
#ifdef _WIN32
    return _putenv_s(name, value.c_str());
#else
    return ::setenv(name, value.c_str(), 1);
#endif
}

inline int unsetEnvVar(const char* name) {
#ifdef _WIN32
    return _putenv_s(name, "");
#else
    return ::unsetenv(name);
#endif
}

class TempDirTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Unique dir name: pid + a global counter + high-res clock + random, so it
        // is collision-free across processes, threads, and test cases.
        static std::atomic<uint64_t> counter{0};
        std::random_device rd;
        std::mt19937_64 gen(static_cast<uint64_t>(rd()) ^
                            std::hash<std::thread::id>{}(std::this_thread::get_id()));
        uint64_t rnd = gen();
        uint64_t seq = counter.fetch_add(1);
        uint64_t now = static_cast<uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());

        std::stringstream ss;
        ss << "stards_test_" << std::hex
           << static_cast<uint64_t>(STARDS_GETPID()) << "_" << seq << "_" << now << "_" << rnd;
        m_tempDir = fs::temp_directory_path() / ss.str();
        fs::create_directories(m_tempDir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);  // best-effort; never throw from TearDown
    }

    // Absolute path to `name` inside this test's private temp directory.
    std::string tempFilePath(const std::string& name = "data.stards") const {
        return (m_tempDir / name).string();
    }

    // A unique .stards path within this test's dir (prefix keeps names readable).
    std::string tempStardsFile(const std::string& prefix = "test") const {
        static std::atomic<uint64_t> n{0};
        std::stringstream ss;
        ss << prefix << "_" << n.fetch_add(1) << ".stards";
        return (m_tempDir / ss.str()).string();
    }

    const fs::path& tempDir() const { return m_tempDir; }

  private:
    fs::path m_tempDir;
};

}  // namespace star_test
