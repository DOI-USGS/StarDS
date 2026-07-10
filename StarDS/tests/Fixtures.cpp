#include "Fixtures.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <filesystem>

using namespace std;


void TempTestingFiles::SetUp() {
  int max_tries = 10;
  auto tmp_dir = fs::temp_directory_path();
  unsigned long long i = 0;
  random_device dev;
  mt19937 prng(dev());
  uniform_int_distribution<uint64_t> rand(0);
  fs::path tpath;

  while (true) {
    stringstream ss;
    ss << "SQTESTS" << hex << rand(prng);
    tpath = tmp_dir / ss.str();

    // true if the directory was created.
    if (fs::create_directory(tpath)) {
        break;
    }
    if (i == max_tries) {
        throw runtime_error("could not find non-existing directory");
    }
    i++;
  }

  tempDir = tpath;

  setenv("SPICEROOT", tempDir.c_str(), true);
  setenv("SPICEQL_CACHE_DIR", tempDir.c_str(), true);
}


void TempTestingFiles::TearDown() {
    if(!fs::remove_all(tempDir)) {
      throw runtime_error("Could not delete temporary files");
    }
}

