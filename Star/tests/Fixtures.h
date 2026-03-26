#pragma once


#include "gtest/gtest.h"
#include <ghc/fs_std.hpp>


using namespace std;


class TempTestingFiles : public ::testing::Environment {
  protected:
    fs::path tempDir;

    void SetUp() override;
    void TearDown() override;
};
