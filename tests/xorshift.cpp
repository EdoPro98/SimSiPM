#include "SiPM.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

using namespace sipm;

struct TestSiPMXorshift256 : public ::testing::Test {};

TEST_F(TestSiPMXorshift256, Constructor) { uint64_t N = 10000000; }

TEST_F(TestSiPMXorshift256, Seed) {
  sipm::SiPMRng::Xorshift256plus rng;
  static constexpr uint64_t seed = 1234567890UL; // Random seed
#ifdef __AVX512F__
  static constexpr uint64_t expected[] = {3539951786562994468ULL,  16993425385450634633ULL, 12425995393443937258ULL,
                                          1971016958421006117ULL,  3113309500227661404ULL,  490387842609610270ULL,
                                          11577763190126509135ULL, 18038816835264277783ULL, 14056837810899630979ULL,
                                          8986600062506074549ULL};
#else

 static constexpr uint64_t expected[] = {2356680413504073166ULL, 6439555299326541142ULL, 13107374383302832124ULL,
                                          15371213951372998008ULL, 3598216317549022935ULL, 9944474804087195216ULL,
                                          1783072794770156681ULL, 9432686255404415156ULL, 5598578073076770953ULL,
                                          11053698116263360353};
#endif
  rng.seed(seed);
  for (int j = 0; j < 10; ++j) {
    uint64_t x = rng();
    EXPECT_EQ(x, expected[j]) << ">> Some error in random generator";
  }
}

TEST_F(TestSiPMXorshift256, AutomaticSeed) {
  sipm::SiPMRng::Xorshift256plus rng1;
  sipm::SiPMRng::Xorshift256plus rng2;
  rng1.seed();
  rng2.seed();
  for (int j = 0; j < 1000; ++j) {
    const uint64_t first = rng1();  // Generate random value
    const uint64_t second = rng2(); // Generate random value
    EXPECT_NE(first, second) << ">> Some error in automatic seed generation";
  }
}

TEST_F(TestSiPMXorshift256, GenerationSmallWindowTest) {
  static constexpr int n = 16;
  uint64_t first_run[n];
  uint64_t second_run[n];

  sipm::SiPMRng::Xorshift256plus rng;

  for (int t = 0; t < 2; ++t) {
    rng.seed(1234567890); // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = rng();
    }
    rng.seed(1234567890); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = rng();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]) << ">> Generator with same seed produces different values";
    }
    for (int i = 0; i < n - 1; ++i) {
      EXPECT_NE(first_run[i], first_run[i + 1])
        << ">> Random number generator has produced two consecutive equal values";
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationMediumWindowTest) {
  static constexpr int n = 256;
  uint64_t first_run[n];
  uint64_t second_run[n];
  sipm::SiPMRng::Xorshift256plus rng;

  for (int t = 0; t < 1024; ++t) {
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
      EXPECT_NE(first_run[i], first_run[i + 1])
        << ">> Random number generator has produced two consecutive equal values";
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationBigWindowTest) {
  static constexpr int n = 65536;
  uint64_t first_run[n];
  uint64_t second_run[n];
  sipm::SiPMRng::Xorshift256plus rng;
  for (int t = 0; t < 1024; ++t) {
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
      EXPECT_NE(first_run[i], first_run[i + 1])
        << ">> Random number generator has produced two consecutive equal values";
    }
  }
}
