#include "SiPM.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

using namespace sipm;

struct TestSiPMXorshift256 : public ::testing::Test {
  static constexpr int N = 1000000;
};

TEST_F(TestSiPMXorshift256, Constructor) {
  uint64_t N = 10000000;
}

TEST_F(TestSiPMXorshift256, Seed) {
  sipm::SiPMRng::Xorshift256plus rng;
  static constexpr uint64_t seed = 1234567890UL; // Random seed
  static constexpr uint64_t expected[] = {
    2408737001010924969ULL, 542980830696407883ULL, 6216539985890296057ULL, 9937196552999892625ULL, 14557266945446803870ULL,
    14778737618991270199ULL, 8040724028886114342ULL, 9480989170591651270ULL, 11279768352260025975ULL, 17595622770473001624ULL};
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
    for (int j = 0; j < 1000; ++j){
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
    rng.seed(seed);               // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = rng();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]) << ">> Generator with same seed produces different values";
    }
    for (int i = 0; i < n - 1; ++i) {
      EXPECT_NE(first_run[i], first_run[i + 1]) << ">> Random number generator has produced two consecutive equal values";
      EXPECT_NE(second_run[i], second_run[i + 1]) << ">> Random number generator has produced two consecutive equal values";
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

TEST_F(TestSiPMXorshift256, ShennonEntropy){
  std::map<uint8_t, uint64_t> counts;
  sipm::SiPMRng::Xorshift256plus rng;
  for(int i=0; i<N; ++i){
    alignas(64) uint8_t bytes[8];
    const uint64_t ull = rng();
    std::memcpy(&bytes, &ull, sizeof(uint64_t));
    for(int j=0; j<8; ++j){
      counts[bytes[j]] += 1;
    }
  }
  double entropy = 0;
  for(const auto& count: counts){
    const double p = (double)count.second / N / 8.0;
    entropy -= p*std::log(p) / std::log(256);
  }
  EXPECT_GE(entropy, 0.95) << ">> Unexpected low entropy from random number generator";
}
