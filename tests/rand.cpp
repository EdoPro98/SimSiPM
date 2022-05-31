#include "SiPM.h"
#include <gtest/gtest.h>
#include <math.h>
#include <stdint.h>

using namespace sipm;

struct TestSiPMRandom : public ::testing::Test {
  static constexpr int N = 10000000;
  static constexpr double muSmall = 0.001;
  static constexpr double muBig = 100;
};

TEST_F(TestSiPMRandom, Constructor) { SiPMRandom rng; }

TEST_F(TestSiPMRandom, RandGeneration) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    double x = rng.Rand();
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 1);
  }
}

TEST_F(TestSiPMRandom, IntegerGeneration) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    uint32_t x = rng.randInteger(100);
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 100);
  }
}

TEST_F(TestSiPMRandom, PoissonGeneration) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    uint32_t x = rng.randPoisson(1);
    EXPECT_GE(x, 0);
  }
}

TEST_F(TestSiPMRandom, ExponentialGeneration) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    double x = rng.randExponential(5);
    EXPECT_GE(x, 0);
  }
}

TEST_F(TestSiPMRandom, NormalGeneration) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    double x = rng.randGaussian(0, 1);
  }
}

TEST_F(TestSiPMRandom, RandGenerationIsRandom) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    double x = rng.Rand();
    double y = rng.Rand();
    EXPECT_TRUE(x != y);
  }
}

TEST_F(TestSiPMRandom, ExponentialGenerationIsRandom) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    double x = rng.randExponential(muSmall);
    double y = rng.randExponential(muSmall);
    EXPECT_TRUE(x != y);
  }
}

TEST_F(TestSiPMRandom, NormalGenerationIsRandom) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {
    double x = rng.randGaussian(0, 1);
    double y = rng.randGaussian(0, 1);
    EXPECT_TRUE(x != y);
  }
}

TEST_F(TestSiPMRandom, RandAverage) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = 1 / std::sqrt(12 * N);
  for (int i = 0; i < N; ++i) {
    x += rng.Rand();
  }
  x = x / N;
  EXPECT_GE(x, 0.5 - 3 * std);
  EXPECT_LE(x, 0.5 + 3 * std);
}

TEST_F(TestSiPMRandom, IntegerAverageSmall) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = 10 / std::sqrt(12 * N);
  for (int i = 0; i < N; ++i) {
    x += rng.randInteger(10);
  }
  x = x / N;
  EXPECT_GE(x, 4.5 - 3 * std);
  EXPECT_LE(x, 4.5 + 3 * std);
}

TEST_F(TestSiPMRandom, IntegerAverageBig) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = 10000 / std::sqrt(12 * N);
  for (int i = 0; i < N; ++i) {
    x += rng.randInteger(10000);
  }
  x = x / N;
  EXPECT_GE(x, 4999.5 - 3 * std);
  EXPECT_LE(x, 4999.5 + 3 * std);
}

TEST_F(TestSiPMRandom, ExponentialAverageSmall) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = muSmall / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += rng.randExponential(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall - 3 * std);
  EXPECT_LE(x, muSmall + 3 * std);
}

TEST_F(TestSiPMRandom, ExponentialAverageBig) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = muBig / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += rng.randExponential(muBig);
  }
  x = x / N;
  EXPECT_GE(x, muBig - 3 * std);
  EXPECT_LE(x, muBig + 3 * std);
}

TEST_F(TestSiPMRandom, PoissonAverageSmall) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = std::sqrt(muSmall) / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += rng.randPoisson(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall - 3 * std);
  EXPECT_LE(x, muSmall + 3 * std);
}

TEST_F(TestSiPMRandom, PoissonAverageBig) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = std::sqrt(muBig) / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += rng.randPoisson(muBig);
  }
  x = x / N;
  EXPECT_GE(x, muBig - 3 * std);
  EXPECT_LE(x, muBig + 3 * std);
}

TEST_F(TestSiPMRandom, NormalAverageSmall) {
  sipm::SiPMRandom rng;
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += rng.randGaussian(0, muSmall);
  }
  x = x / N;
  EXPECT_GE(x, -3 * muSmall);
  EXPECT_LE(x, 3 * muSmall);
}

TEST_F(TestSiPMRandom, NormalAverageBig) {
  sipm::SiPMRandom rng;
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += rng.randGaussian(0, muBig);
  }
  x = x / N;
  EXPECT_GE(x, -3 * muBig);
  EXPECT_LE(x, 3 * muBig);
}

TEST_F(TestSiPMRandom, RandomCorrelation) {
  sipm::SiPMRandom rng;
  double cov = 0;

  for (int i = 0; i < 1000; ++i) {
    double xi = rng.Rand();
    double yi = rng.Rand();
    for (int j = i; j < 1000; ++j) {
      double xj = rng.Rand();
      double yj = rng.Rand();
      cov += (xi - xj) * (yi - yj);
    }
  }
  cov = cov / (1000 * 1000);
  EXPECT_LE(cov, 0.1);
}

TEST_F(TestSiPMRandom, RandomFCorrelation) {
  sipm::SiPMRandom rng;
  float cov = 0;

  for (int i = 0; i < 1000; ++i) {
    float xi = rng.RandF();
    float yi = rng.RandF();
    for (int j = i; j < 1000; ++j) {
      float xj = rng.RandF();
      float yj = rng.RandF();
      cov += (xi - xj) * (yi - yj);
    }
  }
  cov = cov / (1000 * 1000);
  EXPECT_LE(cov, 0.1);
}
