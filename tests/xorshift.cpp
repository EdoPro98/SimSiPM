#include "SiPM.h"
#include <cstdint>
#include <cstring>
#include <cmath>
#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <algorithm>

using namespace sipm;

struct TestSiPMXorshift256 : public ::testing::Test {
  static constexpr uint64_t FIXED_SEED  = 1234567890ULL;
  static constexpr uint64_t FIXED_SEED2 = 9876543210ULL;
  static constexpr uint32_t BUF = 1u << 16; // internal buffer size
};

// ============================================================================
// Construction and seeding
// ============================================================================

TEST_F(TestSiPMXorshift256, ConstructorWithSeedMatchesSeedMethod) {
  sipm::SiPMRng::Xorshift256plus rng_ctor(FIXED_SEED);
  sipm::SiPMRng::Xorshift256plus rng_method;
  rng_method.seed(FIXED_SEED);
  for (int i = 0; i < 10000; ++i) {
    EXPECT_EQ(rng_ctor(), rng_method())
      << "Seeded constructor and seed() diverged at step " << i;
  }
}

TEST_F(TestSiPMXorshift256, SeedKnownOutput) {
  sipm::SiPMRng::Xorshift256plus rng;
#ifdef __AVX512F__
  static constexpr uint64_t expected[] = {3539951786562994468ULL, 16993425385450634633ULL, 12425995393443937258ULL,
                                           1971016958421006117ULL,  3113309500227661404ULL,   490387842609610270ULL,
                                          11577763190126509135ULL, 18038816835264277783ULL, 14056837810899630979ULL,
                                           8986600062506074549ULL};
#elif defined(__AVX2__)
  static constexpr uint64_t expected[] = { 3539951786562994468ULL, 16993425385450634633ULL, 12425995393443937258ULL,
                                            1971016958421006117ULL, 18038816835264277783ULL,  8360080835741225869ULL,
                                            1041359908030902232ULL,  8878485723177124231ULL, 14056837810899630979ULL,
                                            8986600062506074549ULL};
#else
  static constexpr uint64_t expected[] = {2356680413504073166ULL,  6439555299326541142ULL, 13107374383302832124ULL,
                                          15371213951372998008ULL,  3598216317549022935ULL,  9944474804087195216ULL,
                                           1783072794770156681ULL,  9432686255404415156ULL,  5598578073076770953ULL,
                                          11053698116263360353ULL};
#endif
  rng.seed(FIXED_SEED);
  for (int j = 0; j < 10; ++j) {
    EXPECT_EQ(rng(), expected[j]) << "Known-output mismatch at index " << j;
  }
}

TEST_F(TestSiPMXorshift256, SeedZeroEdgeCase) {
  // lcg64(0) = 1, so seed(0) must not yield an all-zero state
  sipm::SiPMRng::Xorshift256plus rng;
  rng.seed(0);
  uint64_t any_nonzero = 0;
  for (int i = 0; i < 1000; ++i) any_nonzero |= rng();
  EXPECT_NE(any_nonzero, 0ULL);
}

TEST_F(TestSiPMXorshift256, SeedReusability) {
  sipm::SiPMRng::Xorshift256plus rng;
  rng.seed(FIXED_SEED);
  uint64_t first[100];
  for (int i = 0; i < 100; ++i) first[i] = rng();
  rng.seed(FIXED_SEED);
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(rng(), first[i]) << "Re-seed replay failed at step " << i;
  }
}

TEST_F(TestSiPMXorshift256, AutomaticSeedProducesUniqueSequences) {
  sipm::SiPMRng::Xorshift256plus rng1, rng2;
  rng1.seed();
  rng2.seed();
  bool anyDiff = false;
  for (int j = 0; j < 1000; ++j) {
    if (rng1() != rng2()) { anyDiff = true; break; }
  }
  EXPECT_TRUE(anyDiff);
}

TEST_F(TestSiPMXorshift256, AutomaticSeedViaConstructorUnique) {
  sipm::SiPMRng::Xorshift256plus rng1;
  sipm::SiPMRng::Xorshift256plus rng2;
  bool anyDiff = false;
  for (int j = 0; j < 1000; ++j) {
    if (rng1() != rng2()) { anyDiff = true; break; }
  }
  EXPECT_TRUE(anyDiff);
}

// ============================================================================
// operator() – statistical properties
// ============================================================================

TEST_F(TestSiPMXorshift256, OperatorOutputMean) {
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  double sum = 0;
  for (int i = 0; i < N; ++i) sum += static_cast<double>(rng());
  const double mean     = sum / N;
  const double expected = static_cast<double>(UINT64_MAX) / 2.0;
  const double tol = 3.0 * (static_cast<double>(UINT64_MAX) / std::sqrt(12.0)) / std::sqrt((double)N);
  EXPECT_NEAR(mean, expected, tol);
}

TEST_F(TestSiPMXorshift256, OperatorHighBitBalance) {
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  int set_count = 0;
  for (int i = 0; i < N; ++i) if (rng() >> 63) ++set_count;
  EXPECT_NEAR(set_count, N / 2, 5.0 * std::sqrt((double)N) / 2.0);
}

TEST_F(TestSiPMXorshift256, OperatorLowBitBalance) {
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  int set_count = 0;
  for (int i = 0; i < N; ++i) if (rng() & 1ULL) ++set_count;
  EXPECT_NEAR(set_count, N / 2, 5.0 * std::sqrt((double)N) / 2.0);
}

// ============================================================================
// Window / buffer boundary determinism
// ============================================================================

TEST_F(TestSiPMXorshift256, GenerationSmallWindowTest) {
  static constexpr int n = 16;
  uint64_t first_run[n], second_run[n];
  sipm::SiPMRng::Xorshift256plus rng;
  for (int t = 0; t < 2; ++t) {
    rng.seed(FIXED_SEED);
    for (int i = 0; i < n; ++i) first_run[i]  = rng();
    rng.seed(FIXED_SEED);
    for (int i = 0; i < n; ++i) second_run[i] = rng();
    for (int i = 0; i < n; ++i)
      EXPECT_EQ(first_run[i], second_run[i]) << "Same-seed divergence at " << i;
    for (int i = 0; i < n - 1; ++i)
      EXPECT_NE(first_run[i], first_run[i + 1]) << "Consecutive equal at " << i;
  }
}

TEST_F(TestSiPMXorshift256, GenerationMediumWindowTest) {
  static constexpr int n = 256;
  uint64_t first_run[n], second_run[n];
  std::mt19937_64 meta_rng(0xDEADBEEFCAFEBABEULL);
  sipm::SiPMRng::Xorshift256plus rng;
  for (int t = 0; t < 1024; ++t) {
    const uint64_t seed = meta_rng() | 1ULL;
    rng.seed(seed);
    for (int i = 0; i < n; ++i) first_run[i]  = rng();
    rng.seed(seed);
    for (int i = 0; i < n; ++i) second_run[i] = rng();
    for (int i = 0; i < n; ++i)     EXPECT_EQ(first_run[i], second_run[i]);
    for (int i = 0; i < n - 1; ++i) EXPECT_NE(first_run[i], first_run[i + 1]);
  }
}

TEST_F(TestSiPMXorshift256, GenerationBigWindowTest) {
  static constexpr int n = 65536;
  std::vector<uint64_t> first_run(n), second_run(n);
  std::mt19937_64 meta_rng(0xCAFEBABEDEADBEEFULL);
  sipm::SiPMRng::Xorshift256plus rng;
  for (int t = 0; t < 1024; ++t) {
    const uint64_t seed = meta_rng() | 1ULL;
    rng.seed(seed);
    for (int i = 0; i < n; ++i) first_run[i]  = rng();
    rng.seed(seed);
    for (int i = 0; i < n; ++i) second_run[i] = rng();
    for (int i = 0; i < n; ++i)     EXPECT_EQ(first_run[i], second_run[i]);
    for (int i = 0; i < n - 1; ++i) EXPECT_NE(first_run[i], first_run[i + 1]);
  }
}

TEST_F(TestSiPMXorshift256, GenerationAcrossMultipleBuffers) {
  const uint32_t total = 4 * BUF;
  std::vector<uint64_t> run1(total), run2(total);
  sipm::SiPMRng::Xorshift256plus rng;
  rng.seed(FIXED_SEED);
  for (uint32_t i = 0; i < total; ++i) run1[i] = rng();
  rng.seed(FIXED_SEED);
  for (uint32_t i = 0; i < total; ++i) run2[i] = rng();
  for (uint32_t i = 0; i < total; ++i)
    EXPECT_EQ(run1[i], run2[i]) << "Multi-buffer divergence at " << i;
}

// ============================================================================
// getRand(uint64_t*, n) – direct batch generation
// ============================================================================

TEST_F(TestSiPMXorshift256, GetRandU64MatchesOperator) {
  sipm::SiPMRng::Xorshift256plus rng_op(FIXED_SEED), rng_batch(FIXED_SEED);
  std::vector<uint64_t> via_op(BUF), via_batch(BUF);
  for (uint32_t i = 0; i < BUF; ++i) via_op[i] = rng_op();
  rng_batch.getRand(via_batch.data(), BUF);
  for (uint32_t i = 0; i < BUF; ++i)
    EXPECT_EQ(via_op[i], via_batch[i]) << "operator() vs getRand mismatch at " << i;
}

TEST_F(TestSiPMXorshift256, GetRandU64SmallN1) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint64_t arr[1];
  rng1.getRand(arr, 1);
  EXPECT_EQ(arr[0], rng2());
}

TEST_F(TestSiPMXorshift256, GetRandU64SmallN2) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint64_t arr[2];
  rng1.getRand(arr, 2);
  EXPECT_EQ(arr[0], rng2());
  EXPECT_EQ(arr[1], rng2());
}

TEST_F(TestSiPMXorshift256, GetRandU64SmallN3) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint64_t arr[3];
  rng1.getRand(arr, 3);
  EXPECT_EQ(arr[0], rng2());
  EXPECT_EQ(arr[1], rng2());
  EXPECT_EQ(arr[2], rng2());
}

TEST_F(TestSiPMXorshift256, GetRandU64ExactBufferSize) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  std::vector<uint64_t> batch(BUF);
  rng1.getRand(batch.data(), BUF);
  for (uint32_t i = 0; i < BUF; ++i)
    EXPECT_EQ(batch[i], rng2()) << "Mismatch at " << i;
}

TEST_F(TestSiPMXorshift256, GetRandU64LargerThanBuffer) {
  const uint32_t n = BUF + 1;
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  std::vector<uint64_t> batch(n);
  rng1.getRand(batch.data(), n);
  for (uint32_t i = 0; i < n; ++i)
    EXPECT_EQ(batch[i], rng2()) << "Mismatch at " << i;
}

TEST_F(TestSiPMXorshift256, GetRandU64Determinism) {
  sipm::SiPMRng::Xorshift256plus rng1, rng2;
  rng1.seed(FIXED_SEED);
  rng2.seed(FIXED_SEED);
  std::vector<uint64_t> a(1000), b(1000);
  rng1.getRand(a.data(), 1000);
  rng2.getRand(b.data(), 1000);
  EXPECT_EQ(a, b);
}

TEST_F(TestSiPMXorshift256, GetRandU64Mean) {
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  std::vector<uint64_t> arr(N);
  rng.getRand(arr.data(), N);
  double sum = 0;
  for (auto v : arr) sum += static_cast<double>(v);
  const double expected = static_cast<double>(UINT64_MAX) / 2.0;
  const double tol = 3.0 * (static_cast<double>(UINT64_MAX) / std::sqrt(12.0)) / std::sqrt((double)N);
  EXPECT_NEAR(sum / N, expected, tol);
}

// ============================================================================
// getRand(uint32_t*, n) – direct batch generation
// ============================================================================

TEST_F(TestSiPMXorshift256, GetRandU32LoBitsHiBitsMatchU64) {
  constexpr int n = 100;
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint64_t u64[n];
  uint32_t u32[2 * n];
  rng1.getRand(u64, n);
  rng2.getRand(u32, 2 * n);
  for (int i = 0; i < n; ++i) {
    EXPECT_EQ(u32[2 * i],     static_cast<uint32_t>(u64[i]))       << "Lo mismatch at " << i;
    EXPECT_EQ(u32[2 * i + 1], static_cast<uint32_t>(u64[i] >> 32)) << "Hi mismatch at " << i;
  }
}

TEST_F(TestSiPMXorshift256, GetRandU32SmallN1) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint32_t arr[1];
  rng1.getRand(arr, 1);
  uint64_t ref;
  rng2.getRand(&ref, 1);
  EXPECT_EQ(arr[0], static_cast<uint32_t>(ref));
}

TEST_F(TestSiPMXorshift256, GetRandU32SmallN2) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint32_t arr[2];
  rng1.getRand(arr, 2);
  uint64_t ref;
  rng2.getRand(&ref, 1);
  EXPECT_EQ(arr[0], static_cast<uint32_t>(ref));
  EXPECT_EQ(arr[1], static_cast<uint32_t>(ref >> 32));
}

TEST_F(TestSiPMXorshift256, GetRandU32SmallN3) {
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  uint32_t arr[3];
  rng1.getRand(arr, 3);
  uint64_t ref[2];
  rng2.getRand(ref, 2);
  EXPECT_EQ(arr[0], static_cast<uint32_t>(ref[0]));
  EXPECT_EQ(arr[1], static_cast<uint32_t>(ref[0] >> 32));
  EXPECT_EQ(arr[2], static_cast<uint32_t>(ref[1]));
}

TEST_F(TestSiPMXorshift256, GetRandU32Determinism) {
  sipm::SiPMRng::Xorshift256plus rng1, rng2;
  rng1.seed(FIXED_SEED);
  rng2.seed(FIXED_SEED);
  std::vector<uint32_t> a(2000), b(2000);
  rng1.getRand(a.data(), (uint32_t)a.size());
  rng2.getRand(b.data(), (uint32_t)b.size());
  EXPECT_EQ(a, b);
}

TEST_F(TestSiPMXorshift256, GetRandU32Mean) {
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  std::vector<uint32_t> arr(N);
  rng.getRand(arr.data(), N);
  double sum = 0;
  for (auto v : arr) sum += static_cast<double>(v);
  const double expected = static_cast<double>(UINT32_MAX) / 2.0;
  const double tol = 3.0 * (static_cast<double>(UINT32_MAX) / std::sqrt(12.0)) / std::sqrt((double)N);
  EXPECT_NEAR(sum / N, expected, tol);
}

TEST_F(TestSiPMXorshift256, GetRandU32ExactDoubleBufferSize) {
  const uint32_t n = 2 * BUF;
  sipm::SiPMRng::Xorshift256plus rng1(FIXED_SEED), rng2(FIXED_SEED);
  std::vector<uint32_t> a(n), b(n);
  rng1.getRand(a.data(), n);
  rng2.getRand(b.data(), n);
  EXPECT_EQ(a, b);
}

// ============================================================================
// Statistical tests
// ============================================================================

TEST_F(TestSiPMXorshift256, StatisticalBitBalanceAllPositions) {
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  std::vector<uint64_t> arr(N);
  rng.getRand(arr.data(), N);
  const double tol = 5.0 * std::sqrt((double)N) / 2.0;
  for (int bit = 0; bit < 64; ++bit) {
    int cnt = 0;
    for (auto v : arr) cnt += static_cast<int>((v >> bit) & 1ULL);
    EXPECT_NEAR(cnt, N / 2, tol) << "Bit " << bit << " imbalanced";
  }
}

TEST_F(TestSiPMXorshift256, ChiSquaredUniformityU64) {
  // Bin top 8 bits into 256 buckets. chi2(255) has mean 255, std sqrt(510).
  constexpr int K = 256;
  constexpr int N = 2000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  int bins[K] = {};
  for (int i = 0; i < N; ++i) bins[rng() >> 56]++;
  double chi2 = 0;
  const double expected = static_cast<double>(N) / K;
  for (int k = 0; k < K; ++k) {
    double d = bins[k] - expected;
    chi2 += d * d / expected;
  }
  EXPECT_NEAR(chi2, K - 1, 5.0 * std::sqrt(2.0 * (K - 1)));
}

TEST_F(TestSiPMXorshift256, ChiSquaredUniformityU32) {
  constexpr int K = 256;
  constexpr int N = 2000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  std::vector<uint32_t> arr(N);
  rng.getRand(arr.data(), N);
  int bins[K] = {};
  for (uint32_t v : arr) bins[v >> 24]++;
  double chi2 = 0;
  const double expected = static_cast<double>(N) / K;
  for (int k = 0; k < K; ++k) {
    double d = bins[k] - expected;
    chi2 += d * d / expected;
  }
  EXPECT_NEAR(chi2, K - 1, 5.0 * std::sqrt(2.0 * (K - 1)));
}

TEST_F(TestSiPMXorshift256, SerialCorrelation) {
  // Pearson lag-1 autocorrelation of normalized u64 values should be ~0.
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  // Scale factor to map u64 → (0,1)
  const double scale = 1.0 / static_cast<double>(UINT64_MAX);
  double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0, sum_y2 = 0;
  double prev = static_cast<double>(rng()) * scale;
  for (int i = 1; i < N; ++i) {
    double curr = static_cast<double>(rng()) * scale;
    sum_x  += prev;   sum_y  += curr;
    sum_xy += prev * curr;
    sum_x2 += prev * prev;
    sum_y2 += curr * curr;
    prev = curr;
  }
  const int M = N - 1;
  const double num  = M * sum_xy - sum_x * sum_y;
  const double den  = std::sqrt((M * sum_x2 - sum_x * sum_x) * (M * sum_y2 - sum_y * sum_y));
  const double corr = num / den;
  // For IID uniform, std(corr) ≈ 1/sqrt(M) ≈ 0.001; use 10-sigma tolerance.
  EXPECT_NEAR(corr, 0.0, 10.0 / std::sqrt((double)M));
}

TEST_F(TestSiPMXorshift256, RunsTestAboveBelowMedian) {
  // Count runs above/below UINT64_MAX/2. Expected ≈ N/2 runs, std ≈ sqrt(N)/2.
  constexpr int N = 1000000;
  sipm::SiPMRng::Xorshift256plus rng(FIXED_SEED);
  const uint64_t median = UINT64_MAX / 2;
  bool above = rng() > median;
  int runs = 1;
  for (int i = 1; i < N; ++i) {
    bool curr = rng() > median;
    if (curr != above) { ++runs; above = curr; }
  }
  const double expected = static_cast<double>(N) / 2.0 + 1.0;
  const double tol      = 5.0 * std::sqrt((double)N) / 2.0;
  EXPECT_NEAR(runs, expected, tol);
}
