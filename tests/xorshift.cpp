#include "SiPM.h"
#include <gtest/gtest.h>

using namespace sipm;

struct TestSiPMXorshift256 : public ::testing::Test {
  static const int N = 1000000;

  SiPMRng::Xorshift256plus sut;
};

TEST_F(TestSiPMXorshift256, Constructor) {
  uint64_t random = sut(); // Generate random value
}

TEST_F(TestSiPMXorshift256, Seed) {
  const uint64_t seed = 1234567890UL; // Random seed
  sut.seed(seed);                     // Set Seed
  uint64_t first = sut();             // Generate random value
  sut.seed(seed);                     // Same seed as before
  uint64_t second = sut();            // Generate random value
  EXPECT_EQ(first, second);
}

TEST_F(TestSiPMXorshift256, AutomaticSeed) {
  for (int i = 0; i < N; ++i) {
    sut.seed();              // Set automatic seed
    uint64_t first = sut();  // Generate random value
    sut.seed();              // Set automatic seed
    uint64_t second = sut(); // Generate random value
    EXPECT_NE(first, second);
  }
}

TEST_F(TestSiPMXorshift256, Generation) {
  const int n = 100;
  uint64_t first_run[n];
  uint64_t second_run[n];

  for (int t = 0; t < N; ++t) {
    uint64_t seed = rand(); // Generate a random seed
    sut.seed(seed);         // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = sut();
    }
    sut.seed(seed); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = sut();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]); // Check that values are equal
    }
  }
}
