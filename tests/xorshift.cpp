#include "SiPM.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

using namespace sipm;

struct TestSiPMXorshift256 : public ::testing::Test {
  static constexpr int N = 1000000;
};

TEST_F(TestSiPMXorshift256, Constructor) { uint64_t N = 10000000; }

TEST_F(TestSiPMXorshift256, Seed) {
  sipm::SiPMRng::Xorshift256plus rng;
  static constexpr uint64_t seed = 1234567890UL; // Random seed
  static constexpr uint64_t expected[] = {7187668669200617491ULL,  17230049399806984457ULL, 10995358304343301012ULL,
                                          17392862592369222144ULL, 2540269431009806966ULL,  17029333743873597984ULL,
                                          2669360090823403747ULL,  4097781474655769227ULL,  13930588530445523158ULL,
                                          7988878967726903275ULL};

  for (int i = 0; i < N; ++i) {
    rng.seed(seed);
    for (int j = 0; j < 10; ++j) {
      uint64_t x = rng();
      EXPECT_EQ(x, expected[j]) << ">> Some error in random generator";
    }
  }
}

TEST_F(TestSiPMXorshift256, AutomaticSeed) {
  sipm::SiPMRng::Xorshift256plus rng1;
  sipm::SiPMRng::Xorshift256plus rng2;
  for (int i = 0; i < 1000; ++i) {
    rng1.seed();
    rng2.seed();
    for (int j = 0; j < 1000; ++j) {
      const uint64_t first = rng1();  // Generate random value
      const uint64_t second = rng2(); // Generate random value
      EXPECT_NE(first, second) << ">> Some error in automatic seed generation";
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationSmallWindowTest) {
  static constexpr int n = 16;
  uint64_t first_run[n];
  uint64_t second_run[n];

  sipm::SiPMRng::Xorshift256plus rng;

  for (int t = 0; t < N; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
    rng.seed(seed);               // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = rng();
    }
    rng.seed(seed); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = rng();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]) << ">> Generator with same seed produces different values";
    }
    for (int i = 0; i < n - 1; ++i) {
      EXPECT_NE(first_run[i], first_run[i + 1])
        << ">> Random number generator has produced two consecutive equal values";
      EXPECT_NE(second_run[i], second_run[i + 1])
        << ">> Random number generator has produced two consecutive equal values";
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationMediumWindowTest) {
  static constexpr int n = 256;
  uint64_t first_run[n];
  uint64_t second_run[n];
  sipm::SiPMRng::Xorshift256plus rng;

  for (int t = 0; t < N; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
    rng.seed(seed);               // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = rng();
    }
    rng.seed(seed); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = rng();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]);
    }
    for (int i = 0; i < n - 1; ++i) {
      EXPECT_NE(first_run[i], first_run[i + 1]);
      EXPECT_NE(second_run[i], second_run[i + 1]);
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationBigWindowTest) {
  static constexpr int n = 65536;
  uint64_t first_run[n];
  uint64_t second_run[n];
  sipm::SiPMRng::Xorshift256plus rng;
  for (int t = 0; t < 1000; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
    rng.seed(seed);               // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = rng();
    }
    rng.seed(seed); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = rng();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]); // Check that values are equal
    }
    for (int i = 0; i < n - 1; ++i) {
      EXPECT_NE(first_run[i], first_run[i + 1]);
      EXPECT_NE(second_run[i], second_run[i + 1]);
    }
  }
}
