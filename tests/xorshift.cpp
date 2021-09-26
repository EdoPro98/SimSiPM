#include "SiPM.h"
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
  static constexpr uint64_t expected[] = {2469135783U,
                                          211107467100884U,
                                          13860777831681755443U,
                                          9331928157278407912U,
                                          3578162886484453791U,
                                          3774445476066261209U,
                                          18428556221801251841U,
                                          16165693134356651351U,
                                          14486711931236482216U,
                                          10419395304814325825U};
  for(int i=0;i<N;++i){
    sut.seed(seed);
    for(int j=0;j<10;++j){
      uint64_t x = sut();
      EXPECT_EQ(x,expected[j]);
    }
  }
}

TEST_F(TestSiPMXorshift256, AutomaticSeed) {
  for (int i = 0; i < 1000000; ++i) {
    sut.seed();              // Set automatic seed
    uint64_t first = sut();  // Generate random value
    sut.seed();              // Set automatic seed
    uint64_t second = sut(); // Generate random value
    EXPECT_NE(first, second);
  }
}

TEST_F(TestSiPMXorshift256, GenerationSmallWindowTest) {
  static constexpr int n = 16;
  uint64_t first_run[n];
  uint64_t second_run[n];

  for (int t = 0; t < N; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
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
    for(int i=0; i<n-1; ++i){
      EXPECT_NE(first_run[i],first_run[i+1]);
      EXPECT_NE(second_run[i],second_run[i+1]);
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationMediumWindowTest) {
  static constexpr int n = 256;
  uint64_t first_run[n];
  uint64_t second_run[n];

  for (int t = 0; t < N; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
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
    for(int i=0; i<n-1; ++i){
      EXPECT_NE(first_run[i],first_run[i+1]);
      EXPECT_NE(second_run[i],second_run[i+1]);
    }
  }
}

TEST_F(TestSiPMXorshift256, GenerationBigWindowTest) {
  static constexpr int n = 65536;
  uint64_t first_run[n];
  uint64_t second_run[n];

  for (int t = 0; t < 1000; ++t) {
    const uint64_t seed = rand(); // Generate a random seed
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
    for(int i=0; i<n-1; ++i){
      EXPECT_NE(first_run[i],first_run[i+1]);
      EXPECT_NE(second_run[i],second_run[i+1]);
    }
  }
}
