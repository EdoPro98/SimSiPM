#include "SiPM.h"
#include <gtest/gtest.h>
#include <stdint.h>

using namespace sipm;

struct TestSiPMRandom : public ::testing::Test {
  static const int N = 1000000;
  double muSmall = 0.001;
  double muBig = 100;
  double toll = 0.01;
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

TEST_F(TestSiPMRandom, PoissonGenerationSmall) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = sut.randPoisson(0.5);
    EXPECT_GE(x, 0);
  }
}

TEST_F(TestSiPMRandom, PoissonGenerationBig) {
  for (int i = 0; i < N; ++i) {
    uint32_t x = sut.randPoisson(100);
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
    double x = sut.randExponential(1);
    double y = sut.randExponential(1);
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
  for (int i = 0; i < N; ++i) {
    x += sut.Rand();
  }
  x = x / N;
  EXPECT_GE(x, 0.5 * (1 - toll));
  EXPECT_LE(x, 0.55 * (1 + toll));
}

TEST_F(TestSiPMRandom, IntegerAverageSmall) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randInteger(10);
  }
  x = x / N;
  EXPECT_GE(x, 5 * (1 - toll));
  EXPECT_LE(x, 5 * (1 + toll));
}

TEST_F(TestSiPMRandom, IntegerAverageBig) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randInteger(1000);
  }
  x = x / N;
  EXPECT_GE(x, 500 * (1 - toll));
  EXPECT_LE(x, 500 * (1 + toll));
}

TEST_F(TestSiPMRandom, ExponentialAverageSmall) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randExponential(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall * (1 - toll));
  EXPECT_LE(x, muSmall * (1 + toll));
}

TEST_F(TestSiPMRandom, ExponentialAverageBig) {
  double mu = 100;
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randExponential(muBig);
  }
  x = x / N;
  EXPECT_GE(x, muBig * (1 - toll));
  EXPECT_LE(x, muBig * (1 + toll));
}

TEST_F(TestSiPMRandom, PoissonAverageSmall) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randPoisson(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall * (1 - toll * 5));
  EXPECT_LE(x, muSmall * (1 + toll * 5));
}

TEST_F(TestSiPMRandom, PoissonAverageBig) {
  double mu = 100;
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randPoisson(muBig);
  }
  x = x / N;
  EXPECT_GE(x, muBig * (1 - toll));
  EXPECT_LE(x, muBig * (1 + toll));
}

TEST_F(TestSiPMRandom, NormalAverageSmall) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randGaussian(0, 1);
  }
  x = x / N;
  EXPECT_GE(x, -toll);
  EXPECT_LE(x, +toll);
}

TEST_F(TestSiPMRandom, NormalAverageBig) {
  double x = 0;
  for (int i = 0; i < N; ++i) {
    x += sut.randGaussian(0, 10);
  }
  x = x / N;
  EXPECT_GE(x, -toll * 3);
  EXPECT_LE(x, +toll * 3);
}
