#include "SiPM.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

using namespace sipm;

struct TestSiPMXorshift256 : public ::testing::Test {
  static constexpr int N = 1000000;
  SiPMRng::Xorshift256plus sut;
};

TEST_F(TestSiPMXorshift256, Constructor) {
  uint64_t random = sut(); // Generate random value
}

TEST_F(TestSiPMXorshift256, Seed) {
  static constexpr uint64_t seed = 1234567890UL; // Random seed
  static constexpr uint64_t expected[] = {
    2469135783U,          211107467100884U,      13860777831681755443U, 9331928157278407912U,  3578162886484453791U,
    3774445476066261209U, 18428556221801251841U, 16165693134356651351U, 14486711931236482216U, 10419395304814325825U};
  for (int i = 0; i < N; ++i) {
    sut.seed(seed);
    for (int j = 0; j < 10; ++j) {
      uint64_t x = sut();
      EXPECT_EQ(x, expected[j]) << ">> Some error in random generator";
    }
  }
}

TEST_F(TestSiPMXorshift256, AutomaticSeed) {
  for (int i = 0; i < 1000; ++i) {
    sut.seed();              // Set automatic seed
    uint64_t first = sut();  // Generate random value
    sut.seed();              // Set automatic seed
    uint64_t second = sut(); // Generate random value
    EXPECT_NE(first, second) << ">> Some error in automatic seed generation";
  }
}

TEST_F(TestSiPMXorshift256, GenerationSmallWindowTest) {
  static constexpr int n = 16;
  uint64_t first_run[n];
  uint64_t second_run[n];

  for (int t = 0; t < N; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
    sut.seed(seed);               // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = sut();
    }
    sut.seed(seed); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = sut();
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

  for (int t = 0; t < N; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
    sut.seed(seed);               // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = sut();
    }
    sut.seed(seed); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = sut();
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

  for (int t = 0; t < 1000; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
    sut.seed(seed);               // Set seed
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
    for (int i = 0; i < n - 1; ++i) {
      EXPECT_NE(first_run[i], first_run[i + 1]);
      EXPECT_NE(second_run[i], second_run[i + 1]);
    }
  }
}

TEST_F(TestSiPMXorshift256, ShennonEntropy){
  std::map<uint8_t, uint64_t> counts;
  for(int i=0; i<N; ++i){
    alignas(64) uint8_t bytes[8];
    const uint64_t ull = sut();
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
