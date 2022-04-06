#include "SiPM.h"
#include <gtest/gtest.h>
#include <math.h>
#include <stdint.h>

using namespace sipm;

struct TestSiPMRandom : public ::testing::Test {
  static constexpr int N = 10000000;
  static constexpr double muSmall = 0.001;
  static constexpr double muBig = 100;
  SiPMRandom sut;
};

TEST_F(TestSiPMRandom, Constructor) { SiPMRandom sut; }

TEST_F(TestSiPMRandom, RandGeneration) {
  for (int i = 0; i < N; ++i) {
    double x = sut.Rand();
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 1);
  }
}

TEST_F(TestSiPMRandom, IntegerGeneration) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = sut.randInteger(100);
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 100);
  }
}

TEST_F(TestSiPMRandom, PoissonGeneration) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = sut.randPoisson(1);
    EXPECT_GE(x, 0);
  }
}

TEST_F(TestSiPMRandom, ExponentialGeneration) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = sut.randExponential(5);
    EXPECT_GE(x, 0);
  }
}

TEST_F(TestSiPMRandom, NormalGeneration) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = sut.randGaussian(0, 1);
  }
}

TEST_F(TestSiPMRandom, RandGenerationIsRandom) {
  for (int i = 0; i < N; ++i) {
    double x = sut.Rand();
    double y = sut.Rand();
    EXPECT_NE(x, y);
  }
}

TEST_F(TestSiPMRandom, ExponentialGenerationIsRandom) {
  for (int i = 0; i < N; ++i) {
    double x = sut.randExponential(muSmall);
    double y = sut.randExponential(muSmall);
    EXPECT_NE(x, y);
  }
}

TEST_F(TestSiPMRandom, NormalGenerationIsRandom) {
  for (int i = 0; i < N; ++i) {
    double x = sut.randGaussian(0, 1);
    double y = sut.randGaussian(0, 1);
    EXPECT_NE(x, y);
  }
}

TEST_F(TestSiPMRandom, RandAverage) {
  double x = 0;
  static const double std = 1 / std::sqrt(12 * N);
  for (int i = 0; i < N; ++i) {
    x += sut.Rand();
  }
  x = x / N;
  EXPECT_GE(x, 0.5 - 3 * std);
  EXPECT_LE(x, 0.5 + 3 * std);
}

TEST_F(TestSiPMRandom, IntegerAverageSmall) {
  double x = 0;
  static const double std = 10 / std::sqrt(12 * N);
  for (int i = 0; i < N; ++i) {
    x += sut.randInteger(10);
  }
  x = x / N;
  EXPECT_GE(x, 4.5 - 3 * std);
  EXPECT_LE(x, 4.5 + 3 * std);
}

TEST_F(TestSiPMRandom, IntegerAverageBig) {
  double x = 0;
  static const double std = 10000 / std::sqrt(12 * N);
  for (int i = 0; i < N; ++i) {
    x += sut.randInteger(10000);
  }
  x = x / N;
  EXPECT_GE(x, 4999.5 - 3 * std);
  EXPECT_LE(x, 4999.5 + 3 * std);
}

TEST_F(TestSiPMRandom, ExponentialAverageSmall) {
  double x = 0;
  static const double std = muSmall / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += sut.randExponential(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall - 3 * std);
  EXPECT_LE(x, muSmall + 3 * std);
}

TEST_F(TestSiPMRandom, ExponentialAverageBig) {
  double x = 0;
  static const double std = muBig / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += sut.randExponential(muBig);
  }
  x = x / N;
  EXPECT_GE(x, muBig - 3 * std);
  EXPECT_LE(x, muBig + 3 * std);
}

TEST_F(TestSiPMRandom, PoissonAverageSmall) {
  double x = 0;
  static const double std = std::sqrt(muSmall) / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += sut.randPoisson(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall - 3 * std);
  EXPECT_LE(x, muSmall + 3 * std);
}

TEST_F(TestSiPMRandom, PoissonAverageBig) {
  double x = 0;
  static const double std = std::sqrt(muBig) / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += sut.randPoisson(muBig);
  }
  x = x / N;
  EXPECT_GE(x, muBig - 3 * std);
  EXPECT_LE(x, muBig + 3 * std);
}

TEST_F(TestSiPMRandom, NormalAverageSmall) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randGaussian(0, muSmall);
  }
  x = x / N;
  EXPECT_GE(x, -3 * muSmall);
  EXPECT_LE(x, 3 * muSmall);
}

TEST_F(TestSiPMRandom, NormalAverageBig) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randGaussian(0, muBig);
  }
  x = x / N;
  EXPECT_GE(x, -3 * muBig);
  EXPECT_LE(x, 3 * muBig);
}

TEST_F(TestSiPMRandom, RandomCorrelation) {
  double cov = 0;

  for (int i = 0; i < 1000; ++i) {
    double xi = sut.Rand();
    double yi = sut.Rand();
    for (int j = i; j < 1000; ++j) {
      double xj = sut.Rand();
      double yj = sut.Rand();
      cov += (xi - xj) * (yi - yj);
    }
  }
  cov = cov / (1000 * 1000);
  EXPECT_LE(cov, 0.1);
}
