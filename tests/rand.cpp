#include "SiPM.h"
#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace sipm;

struct TestSiPMRandom : public ::testing::Test {
  static constexpr int N      = 10000000;
  static constexpr int N_VEC  = 1000000;
  static constexpr double muSmall = 0.001;
  static constexpr double muBig   = 100;
  SiPMRandom rng;

  void SetUp() override { rng.seed(42); }
};

// ============================================================================
// Construction and seeding
// ============================================================================

TEST_F(TestSiPMRandom, Constructor) { SiPMRandom r; }

TEST_F(TestSiPMRandom, SeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(12345);
  r2.seed(12345);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.Rand(), r2.Rand());
}

// ============================================================================
// Rand<double>() – uniform [0, 1)
// ============================================================================

TEST_F(TestSiPMRandom, RandRangeInBounds) {
  for (int i = 0; i < N; ++i) {
    double x = rng.Rand();
    EXPECT_GE(x, 0.0);
    EXPECT_LE(x, 1.0);
  }
}

TEST_F(TestSiPMRandom, RandAverage) {
  double sum = 0;
  static const double tol = 3.0 / std::sqrt(12.0 * N);
  for (int i = 0; i < N; ++i) sum += rng.Rand();
  EXPECT_NEAR(sum / N, 0.5, tol);
}

TEST_F(TestSiPMRandom, RandVariance) {
  double sum = 0, sum2 = 0;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.Rand();
    sum  += x;
    sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, 1.0 / 12.0, 5.0 * (1.0 / 12.0) * std::sqrt(2.0 / N_VEC));
}

TEST_F(TestSiPMRandom, RandChiSquaredUniformity) {
  // Bin N_VEC samples into K buckets; chi2(K-1) has mean K-1, std sqrt(2*(K-1)).
  constexpr int K = 100;
  constexpr double expected = static_cast<double>(N_VEC) / K;
  int bins[K] = {};
  for (int i = 0; i < N_VEC; ++i) {
    int b = static_cast<int>(rng.Rand() * K);
    if (b >= K) b = K - 1;
    ++bins[b];
  }
  double chi2 = 0;
  for (int k = 0; k < K; ++k) { double d = bins[k] - expected; chi2 += d * d / expected; }
  EXPECT_NEAR(chi2, K - 1, 5.0 * std::sqrt(2.0 * (K - 1)));
}

TEST_F(TestSiPMRandom, RandSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(777);
  r2.seed(777);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.Rand(), r2.Rand());
}

// ============================================================================
// Rand<float>() – uniform float [0, 1)
// ============================================================================

TEST_F(TestSiPMRandom, RandFloatRangeInBounds) {
  for (int i = 0; i < N; ++i) {
    float x = rng.Rand<float>();
    EXPECT_GE(x, 0.0f);
    EXPECT_LE(x, 1.0f);
  }
}

TEST_F(TestSiPMRandom, RandFloatAverage) {
  double sum = 0;
  static const double tol = 3.0 / std::sqrt(12.0 * N);
  for (int i = 0; i < N; ++i) sum += rng.Rand<float>();
  EXPECT_NEAR(sum / N, 0.5, tol);
}

TEST_F(TestSiPMRandom, RandFloatVariance) {
  double sum = 0, sum2 = 0;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.Rand<float>();
    sum  += x;
    sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, 1.0 / 12.0, 5.0 * (1.0 / 12.0) * std::sqrt(2.0 / N_VEC));
}

TEST_F(TestSiPMRandom, RandFloatChiSquaredUniformity) {
  constexpr int K = 100;
  constexpr double expected = static_cast<double>(N_VEC) / K;
  int bins[K] = {};
  for (int i = 0; i < N_VEC; ++i) {
    int b = static_cast<int>(rng.Rand<float>() * K);
    if (b >= K) b = K - 1;
    ++bins[b];
  }
  double chi2 = 0;
  for (int k = 0; k < K; ++k) { double d = bins[k] - expected; chi2 += d * d / expected; }
  EXPECT_NEAR(chi2, K - 1, 5.0 * std::sqrt(2.0 * (K - 1)));
}

TEST_F(TestSiPMRandom, RandFloatSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(777);
  r2.seed(777);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.Rand<float>(), r2.Rand<float>());
}

// ============================================================================
// RandF2() – two floats packed from one RNG call
// ============================================================================

TEST_F(TestSiPMRandom, RandF2RangeInBounds) {
  for (int i = 0; i < N_VEC; ++i) {
    auto p = rng.RandF2();
    EXPECT_GE(p.first,  0.0f); EXPECT_LE(p.first,  1.0f);
    EXPECT_GE(p.second, 0.0f); EXPECT_LE(p.second, 1.0f);
  }
}

TEST_F(TestSiPMRandom, RandF2AverageFirst) {
  double sum = 0;
  static const double tol = 3.0 / std::sqrt(12.0 * N_VEC);
  for (int i = 0; i < N_VEC; ++i) sum += rng.RandF2().first;
  EXPECT_NEAR(sum / N_VEC, 0.5, tol);
}

TEST_F(TestSiPMRandom, RandF2AverageSecond) {
  double sum = 0;
  static const double tol = 3.0 / std::sqrt(12.0 * N_VEC);
  for (int i = 0; i < N_VEC; ++i) sum += rng.RandF2().second;
  EXPECT_NEAR(sum / N_VEC, 0.5, tol);
}

TEST_F(TestSiPMRandom, RandF2SeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(888);
  r2.seed(888);
  for (int i = 0; i < 1000; ++i) {
    auto p1 = r1.RandF2(); auto p2 = r2.RandF2();
    EXPECT_EQ(p1.first, p2.first); EXPECT_EQ(p1.second, p2.second);
  }
}

// ============================================================================
// randInteger(max) – scalar
// ============================================================================

TEST_F(TestSiPMRandom, IntegerRangeStrictlyLessThanMax) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = rng.randInteger(100);
    EXPECT_GE(x, 0u); EXPECT_LT(x, 100u);
  }
}

TEST_F(TestSiPMRandom, IntegerAverageSmall) {
  double sum = 0;
  static const double tol = 3.0 * 10.0 / std::sqrt(12.0 * N);
  for (int i = 0; i < N; ++i) sum += rng.randInteger(10);
  EXPECT_NEAR(sum / N, 4.5, tol);
}

TEST_F(TestSiPMRandom, IntegerAverageBig) {
  double sum = 0;
  static const double tol = 3.0 * 10000.0 / std::sqrt(12.0 * N);
  for (int i = 0; i < N; ++i) sum += rng.randInteger(10000);
  EXPECT_NEAR(sum / N, 4999.5, tol);
}

TEST_F(TestSiPMRandom, IntegerChiSquaredUniformity) {
  constexpr uint32_t K = 50;
  constexpr double expected = static_cast<double>(N_VEC) / K;
  int bins[K] = {};
  for (int i = 0; i < N_VEC; ++i) ++bins[rng.randInteger(K)];
  double chi2 = 0;
  for (uint32_t k = 0; k < K; ++k) { double d = bins[k] - expected; chi2 += d * d / expected; }
  EXPECT_NEAR(chi2, K - 1, 5.0 * std::sqrt(2.0 * (K - 1)));
}

TEST_F(TestSiPMRandom, IntegerSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(123); r2.seed(123);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.randInteger(1000), r2.randInteger(1000));
}

// ============================================================================
// randInteger2(max) – pair of integers from one RNG call
// ============================================================================

TEST_F(TestSiPMRandom, Integer2RangeInBounds) {
  for (int i = 0; i < N_VEC; ++i) {
    auto p = rng.randInteger2(100);
    EXPECT_LT(p.first, 100u); EXPECT_LT(p.second, 100u);
  }
}

TEST_F(TestSiPMRandom, Integer2AverageFirst) {
  double sum = 0;
  static const double tol = 3.0 * 10.0 / std::sqrt(12.0 * N_VEC);
  for (int i = 0; i < N_VEC; ++i) sum += rng.randInteger2(10).first;
  EXPECT_NEAR(sum / N_VEC, 4.5, tol);
}

TEST_F(TestSiPMRandom, Integer2AverageSecond) {
  double sum = 0;
  static const double tol = 3.0 * 10.0 / std::sqrt(12.0 * N_VEC);
  for (int i = 0; i < N_VEC; ++i) sum += rng.randInteger2(10).second;
  EXPECT_NEAR(sum / N_VEC, 4.5, tol);
}

TEST_F(TestSiPMRandom, Integer2SeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(124); r2.seed(124);
  for (int i = 0; i < 1000; ++i) {
    auto p1 = r1.randInteger2(1000); auto p2 = r2.randInteger2(1000);
    EXPECT_EQ(p1.first, p2.first); EXPECT_EQ(p1.second, p2.second);
  }
}

// ============================================================================
// randExponential(mu) – double scalar
// ============================================================================

TEST_F(TestSiPMRandom, ExponentialPositive) {
  for (int i = 0; i < N; ++i) EXPECT_GE(rng.randExponential(5.0), 0.0);
}

TEST_F(TestSiPMRandom, ExponentialAverageSmall) {
  double sum = 0;
  static const double tol = 3.0 * muSmall / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randExponential(muSmall);
  EXPECT_NEAR(sum / N, muSmall, tol);
}

TEST_F(TestSiPMRandom, ExponentialAverageBig) {
  double sum = 0;
  static const double tol = 3.0 * muBig / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randExponential(muBig);
  EXPECT_NEAR(sum / N, muBig, tol);
}

TEST_F(TestSiPMRandom, ExponentialVariance) {
  double sum = 0, sum2 = 0;
  const double mu = 5.0;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.randExponential(mu); sum += x; sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, mu * mu, 5.0 * mu * mu * std::sqrt(2.0 / N_VEC));
}

TEST_F(TestSiPMRandom, ExponentialCDF) {
  // P(X > k*mu) = exp(-k) for k = 1, 2, 3
  const double mu = 5.0;
  int above[3] = {};
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.randExponential(mu);
    if (x > mu)     ++above[0];
    if (x > 2 * mu) ++above[1];
    if (x > 3 * mu) ++above[2];
  }
  for (int k = 1; k <= 3; ++k) {
    const double p   = std::exp(-(double)k);
    const double tol = 5.0 * std::sqrt(p * (1.0 - p) / N_VEC);
    EXPECT_NEAR(above[k - 1] / (double)N_VEC, p, tol) << "k=" << k;
  }
}

TEST_F(TestSiPMRandom, ExponentialSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(456); r2.seed(456);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.randExponential(3.14), r2.randExponential(3.14));
}

// ============================================================================
// randExponentialF(mu) – float scalar
// ============================================================================

TEST_F(TestSiPMRandom, ExponentialFPositive) {
  for (int i = 0; i < N; ++i) EXPECT_GE(rng.randExponentialF(5.0f), 0.0f);
}

TEST_F(TestSiPMRandom, ExponentialFAverageSmall) {
  static const float muF = 0.001f;
  static const int Ns = 2000;
  static const double tol = 5.0 * muF / std::sqrt((double)Ns);
  SiPMRandom r; r.seed(1001);
  double sum = 0;
  for (int i = 0; i < Ns; ++i) sum += r.randExponentialF(muF);
  EXPECT_NEAR(sum / Ns, muF, tol);
}

TEST_F(TestSiPMRandom, ExponentialFAverageBig) {
  static const float muF = 100.0f;
  static const int Ns = 2000;
  static const double tol = 5.0 * muF / std::sqrt((double)Ns);
  SiPMRandom r; r.seed(1002);
  double sum = 0;
  for (int i = 0; i < Ns; ++i) sum += r.randExponentialF(muF);
  EXPECT_NEAR(sum / Ns, muF, tol);
}

TEST_F(TestSiPMRandom, ExponentialFVariance) {
  double sum = 0, sum2 = 0;
  const float mu = 5.0f;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.randExponentialF(mu); sum += x; sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, (double)mu * mu, 5.0 * (double)mu * mu * std::sqrt(2.0 / N_VEC));
}

TEST_F(TestSiPMRandom, ExponentialFSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(456); r2.seed(456);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.randExponentialF(3.14f), r2.randExponentialF(3.14f));
}

// ============================================================================
// randGaussian(mu, sigma) – double scalar
// ============================================================================

TEST_F(TestSiPMRandom, GaussianMeanZero) {
  double sum = 0;
  static const double tol = 3.0 / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randGaussian(0.0, 1.0);
  EXPECT_NEAR(sum / N, 0.0, tol);
}

TEST_F(TestSiPMRandom, GaussianMeanWithOffset) {
  double sum = 0;
  const double mu = 5.0, sigma = 2.0;
  static const double tol = 3.0 * sigma / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randGaussian(mu, sigma);
  EXPECT_NEAR(sum / N, mu, tol);
}

TEST_F(TestSiPMRandom, GaussianVariance) {
  double sum = 0, sum2 = 0;
  const double sigma = 3.0;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.randGaussian(0.0, sigma); sum += x; sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, sigma * sigma, 5.0 * sigma * sigma * std::sqrt(2.0 / N_VEC));
}

TEST_F(TestSiPMRandom, GaussianSigmaCoverage) {
  // Empirical rule: 68.27% / 95.45% / 99.73% within 1/2/3 sigma.
  const int Nv = N_VEC;
  int w1 = 0, w2 = 0, w3 = 0;
  for (int i = 0; i < Nv; ++i) {
    double x = std::abs(rng.randGaussian(0.0, 1.0));
    if (x < 1.0) ++w1;
    if (x < 2.0) ++w2;
    if (x < 3.0) ++w3;
  }
  const double p1 = 0.6827, p2 = 0.9545, p3 = 0.9973;
  EXPECT_NEAR(w1 / (double)Nv, p1, 5.0 * std::sqrt(p1 * (1 - p1) / Nv));
  EXPECT_NEAR(w2 / (double)Nv, p2, 5.0 * std::sqrt(p2 * (1 - p2) / Nv));
  EXPECT_NEAR(w3 / (double)Nv, p3, 5.0 * std::sqrt(p3 * (1 - p3) / Nv));
}

TEST_F(TestSiPMRandom, GaussianSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(789); r2.seed(789);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.randGaussian(0, 1), r2.randGaussian(0, 1));
}

// ============================================================================
// randGaussianF(mu, sigma) – float scalar
// ============================================================================

TEST_F(TestSiPMRandom, GaussianFMeanZero) {
  double sum = 0;
  static const double tol = 3.0 / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randGaussianF(0.0f, 1.0f);
  EXPECT_NEAR(sum / N, 0.0, tol);
}

TEST_F(TestSiPMRandom, GaussianFMeanWithOffset) {
  double sum = 0;
  const float mu = 5.0f, sigma = 2.0f;
  static const double tol = 3.0 * sigma / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randGaussianF(mu, sigma);
  EXPECT_NEAR(sum / N, (double)mu, tol);
}

TEST_F(TestSiPMRandom, GaussianFVariance) {
  double sum = 0, sum2 = 0;
  const float sigma = 3.0f;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.randGaussianF(0.0f, sigma); sum += x; sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, (double)sigma * sigma, 8.0 * (double)sigma * sigma * std::sqrt(2.0 / N_VEC));
}

TEST_F(TestSiPMRandom, GaussianFSigmaCoverage) {
  const int Nv = N_VEC;
  int w1 = 0, w2 = 0, w3 = 0;
  for (int i = 0; i < Nv; ++i) {
    float x = std::abs(rng.randGaussianF(0.0f, 1.0f));
    if (x < 1.0f) ++w1;
    if (x < 2.0f) ++w2;
    if (x < 3.0f) ++w3;
  }
  const double p1 = 0.6827, p2 = 0.9545, p3 = 0.9973;
  EXPECT_NEAR(w1 / (double)Nv, p1, 5.0 * std::sqrt(p1 * (1 - p1) / Nv));
  EXPECT_NEAR(w2 / (double)Nv, p2, 5.0 * std::sqrt(p2 * (1 - p2) / Nv));
  EXPECT_NEAR(w3 / (double)Nv, p3, 5.0 * std::sqrt(p3 * (1 - p3) / Nv));
}

TEST_F(TestSiPMRandom, GaussianFSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(789); r2.seed(789);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.randGaussianF(0.0f, 1.0f), r2.randGaussianF(0.0f, 1.0f));
}

// ============================================================================
// randPoisson(mu)
// ============================================================================

TEST_F(TestSiPMRandom, PoissonGeneration) {
  SiPMRandom r;
  for (int i = 0; i < N; ++i) EXPECT_GE(r.randPoisson(1), 0u);
}

TEST_F(TestSiPMRandom, PoissonAverageSmall) {
  double sum = 0;
  static const double tol = 3.0 * std::sqrt(muSmall) / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randPoisson(muSmall);
  EXPECT_NEAR(sum / N, muSmall, tol);
}

TEST_F(TestSiPMRandom, PoissonAverageBig) {
  double sum = 0;
  static const double tol = 3.0 * std::sqrt(muBig) / std::sqrt((double)N);
  for (int i = 0; i < N; ++i) sum += rng.randPoisson(muBig);
  EXPECT_NEAR(sum / N, muBig, tol);
}

TEST_F(TestSiPMRandom, PoissonVariance) {
  // For Poisson(mu), variance = mu
  double sum = 0, sum2 = 0;
  const double mu = 5.0;
  for (int i = 0; i < N_VEC; ++i) {
    double x = rng.randPoisson(mu); sum += x; sum2 += x * x;
  }
  const double var = sum2 / N_VEC - (sum / N_VEC) * (sum / N_VEC);
  EXPECT_NEAR(var, mu, 5.0 * std::sqrt(2.0 * mu * mu / N_VEC));
}

TEST_F(TestSiPMRandom, PoissonPMF) {
  // P(X=k) = e^{-mu} * mu^k / k! for mu=5, k=0..14
  const double mu = 5.0;
  std::vector<int> counts(20, 0);
  for (int i = 0; i < N_VEC; ++i) {
    uint32_t x = rng.randPoisson(mu);
    if (x < 20) ++counts[x];
  }
  double pmf = std::exp(-mu); // P(X=0)
  for (int k = 0; k < 15; ++k) {
    const double expected_count = pmf * N_VEC;
    const double tol = 5.0 * std::sqrt(pmf * (1.0 - pmf) * N_VEC);
    EXPECT_NEAR(counts[k], expected_count, tol) << "PMF mismatch at k=" << k;
    pmf *= mu / (k + 1);
  }
}

TEST_F(TestSiPMRandom, PoissonZeroMuReturnsZero) {
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(rng.randPoisson(0.0), 0u);
}

TEST_F(TestSiPMRandom, PoissonNegativeMuReturnsZero) {
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(rng.randPoisson(-1.0), 0u);
}

TEST_F(TestSiPMRandom, PoissonSeedDeterminism) {
  SiPMRandom r1, r2;
  r1.seed(321); r2.seed(321);
  for (int i = 0; i < 1000; ++i) EXPECT_EQ(r1.randPoisson(5.0), r2.randPoisson(5.0));
}

// ============================================================================
// Vector: Rand(n)
// ============================================================================

TEST_F(TestSiPMRandom, RandVectorSize)     { EXPECT_EQ(rng.Rand(1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandVectorRange) {
  for (double x : rng.Rand((uint32_t)N_VEC)) { EXPECT_GE(x, 0.0); EXPECT_LE(x, 1.0); }
}
TEST_F(TestSiPMRandom, RandVectorAverage) {
  auto v = rng.Rand((uint32_t)N_VEC);
  EXPECT_NEAR(std::accumulate(v.begin(), v.end(), 0.0) / N_VEC, 0.5,
              3.0 / std::sqrt(12.0 * N_VEC));
}
TEST_F(TestSiPMRandom, RandVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(1111); r2.seed(1111);
  auto v1 = r1.Rand(1000u), v2 = r2.Rand(1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Vector: RandF(n)
// ============================================================================

TEST_F(TestSiPMRandom, RandFVectorSize)    { EXPECT_EQ(rng.RandF(1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandFVectorRange) {
  for (float x : rng.RandF((uint32_t)N_VEC)) { EXPECT_GE(x, 0.0f); EXPECT_LE(x, 1.0f); }
}
TEST_F(TestSiPMRandom, RandFVectorAverage) {
  auto v = rng.RandF((uint32_t)N_VEC);
  double sum = 0; for (float x : v) sum += x;
  EXPECT_NEAR(sum / N_VEC, 0.5, 3.0 / std::sqrt(12.0 * N_VEC));
}
TEST_F(TestSiPMRandom, RandFVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(2222); r2.seed(2222);
  auto v1 = r1.RandF(1000u), v2 = r2.RandF(1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Vector: randGaussian(mu, sigma, n)
// ============================================================================

TEST_F(TestSiPMRandom, RandGaussianVectorSize) { EXPECT_EQ(rng.randGaussian(0.0, 1.0, 1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandGaussianVectorMeanZero) {
  auto v = rng.randGaussian(0.0, 1.0, (uint32_t)N_VEC);
  EXPECT_NEAR(std::accumulate(v.begin(), v.end(), 0.0) / N_VEC, 0.0,
              3.0 / std::sqrt((double)N_VEC));
}
TEST_F(TestSiPMRandom, RandGaussianVectorMeanWithOffset) {
  const double mu = 7.5, sigma = 2.0;
  auto v = rng.randGaussian(mu, sigma, (uint32_t)N_VEC);
  EXPECT_NEAR(std::accumulate(v.begin(), v.end(), 0.0) / N_VEC, mu,
              3.0 * sigma / std::sqrt((double)N_VEC));
}
TEST_F(TestSiPMRandom, RandGaussianVectorVariance) {
  const double sigma = 2.5;
  auto v = rng.randGaussian(0.0, sigma, (uint32_t)N_VEC);
  const double mean = std::accumulate(v.begin(), v.end(), 0.0) / N_VEC;
  double var = 0; for (double x : v) var += (x - mean) * (x - mean);
  var /= N_VEC;
  EXPECT_NEAR(var, sigma * sigma, 5.0 * sigma * sigma * std::sqrt(2.0 / N_VEC));
}
TEST_F(TestSiPMRandom, RandGaussianVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(3333); r2.seed(3333);
  auto v1 = r1.randGaussian(0.0, 1.0, 1000u), v2 = r2.randGaussian(0.0, 1.0, 1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Vector: randGaussianF(mu, sigma, n)
// ============================================================================

TEST_F(TestSiPMRandom, RandGaussianFVectorSize) { EXPECT_EQ(rng.randGaussianF(0.0f, 1.0f, 1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandGaussianFVectorMeanZero) {
  auto v = rng.randGaussianF(0.0f, 1.0f, (uint32_t)N_VEC);
  double sum = 0; for (float x : v) sum += x;
  EXPECT_NEAR(sum / N_VEC, 0.0, 3.0 / std::sqrt((double)N_VEC));
}
TEST_F(TestSiPMRandom, RandGaussianFVectorMeanWithOffset) {
  const float mu = 7.5f, sigma = 2.0f;
  auto v = rng.randGaussianF(mu, sigma, (uint32_t)N_VEC);
  double sum = 0; for (float x : v) sum += x;
  EXPECT_NEAR(sum / N_VEC, (double)mu, 3.0 * sigma / std::sqrt((double)N_VEC));
}
TEST_F(TestSiPMRandom, RandGaussianFVectorVariance) {
  const float sigma = 2.5f;
  auto v = rng.randGaussianF(0.0f, sigma, (uint32_t)N_VEC);
  double sum = 0; for (float x : v) sum += x;
  const double mean = sum / N_VEC;
  double var = 0; for (float x : v) var += ((double)x - mean) * ((double)x - mean);
  var /= N_VEC;
  EXPECT_NEAR(var, (double)sigma * sigma, 8.0 * (double)sigma * sigma * std::sqrt(2.0 / N_VEC));
}
TEST_F(TestSiPMRandom, RandGaussianFVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(4444); r2.seed(4444);
  auto v1 = r1.randGaussianF(0.0f, 1.0f, 1000u), v2 = r2.randGaussianF(0.0f, 1.0f, 1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Vector: randInteger(max, n)
// ============================================================================

TEST_F(TestSiPMRandom, RandIntegerVectorSize)  { EXPECT_EQ(rng.randInteger(100u, 1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandIntegerVectorRange) {
  for (uint32_t x : rng.randInteger(100u, (uint32_t)N_VEC)) EXPECT_LT(x, 100u);
}
TEST_F(TestSiPMRandom, RandIntegerVectorAverage) {
  auto v = rng.randInteger(10u, (uint32_t)N_VEC);
  double sum = 0; for (uint32_t x : v) sum += x;
  EXPECT_NEAR(sum / N_VEC, 4.5, 3.0 * 10.0 / std::sqrt(12.0 * N_VEC));
}
TEST_F(TestSiPMRandom, RandIntegerVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(5555); r2.seed(5555);
  auto v1 = r1.randInteger(100u, 1000u), v2 = r2.randInteger(100u, 1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Vector: randExponential(mu, n)
// ============================================================================

TEST_F(TestSiPMRandom, RandExponentialVectorSize)  { EXPECT_EQ(rng.randExponential(1.0, 1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandExponentialVectorRange) {
  for (double x : rng.randExponential(5.0, (uint32_t)N_VEC)) EXPECT_GE(x, 0.0);
}
TEST_F(TestSiPMRandom, RandExponentialVectorAverage) {
  const double mu = 5.0;
  auto v = rng.randExponential(mu, (uint32_t)N_VEC);
  EXPECT_NEAR(std::accumulate(v.begin(), v.end(), 0.0) / N_VEC, mu,
              3.0 * mu / std::sqrt((double)N_VEC));
}
TEST_F(TestSiPMRandom, RandExponentialVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(6666); r2.seed(6666);
  auto v1 = r1.randExponential(5.0, 1000u), v2 = r2.randExponential(5.0, 1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Vector: randExponentialF(mu, n)
// ============================================================================

TEST_F(TestSiPMRandom, RandExponentialFVectorSize)  { EXPECT_EQ(rng.randExponentialF(1.0f, 1000u).size(), 1000u); }
TEST_F(TestSiPMRandom, RandExponentialFVectorRange) {
  for (float x : rng.randExponentialF(5.0f, (uint32_t)N_VEC)) EXPECT_GE(x, 0.0f);
}
TEST_F(TestSiPMRandom, RandExponentialFVectorAverage) {
  const float mu = 5.0f;
  auto v = rng.randExponentialF(mu, (uint32_t)N_VEC);
  double sum = 0; for (float x : v) sum += x;
  EXPECT_NEAR(sum / N_VEC, (double)mu, 3.0 * mu / std::sqrt((double)N_VEC));
}
TEST_F(TestSiPMRandom, RandExponentialFVectorSeedDeterminism) {
  SiPMRandom r1, r2; r1.seed(7777); r2.seed(7777);
  auto v1 = r1.randExponentialF(5.0f, 1000u), v2 = r2.randExponentialF(5.0f, 1000u);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) EXPECT_EQ(v1[i], v2[i]);
}

// ============================================================================
// Correlation
// ============================================================================

TEST_F(TestSiPMRandom, RandomCorrelation) {
  double cov = 0;
  for (int i = 0; i < 1000; ++i) {
    double xi = rng.Rand(), yi = rng.Rand();
    for (int j = i; j < 1000; ++j) {
      double xj = rng.Rand(), yj = rng.Rand();
      cov += (xi - xj) * (yi - yj);
    }
  }
  EXPECT_LE(cov / (1000 * 1000), 0.1);
}

TEST_F(TestSiPMRandom, RandomFCorrelation) {
  float cov = 0;
  for (int i = 0; i < 1000; ++i) {
    float xi = rng.Rand<float>(), yi = rng.Rand<float>();
    for (int j = i; j < 1000; ++j) {
      float xj = rng.Rand<float>(), yj = rng.Rand<float>();
      cov += (xi - xj) * (yi - yj);
    }
  }
  EXPECT_LE(cov / (1000 * 1000), 0.1);
}
