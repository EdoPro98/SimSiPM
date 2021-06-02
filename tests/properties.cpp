#include "SiPM.h"
#include <gtest/gtest.h>
#include <stdint.h>

using namespace sipm;

struct TestSiPMProperties : public ::testing::Test {
  SiPMProperties sut;
  SiPMRandom rng;
};

TEST_F(TestSiPMProperties, Constructor) {
  EXPECT_EQ(sut.size(), 1);
  EXPECT_EQ(sut.pitch(), 25);
  EXPECT_EQ(sut.nCells(), 1600);
  EXPECT_DOUBLE_EQ(sut.sampling(), 1);
  EXPECT_EQ(sut.signalLength(), 500);
  EXPECT_EQ(sut.nSignalPoints(), 500);
  EXPECT_DOUBLE_EQ(sut.risingTime(), 1);
  EXPECT_DOUBLE_EQ(sut.fallingTimeFast(), 50);
  EXPECT_DOUBLE_EQ(sut.recoveryTime(), 50);
  EXPECT_DOUBLE_EQ(sut.dcr(), 200e3);
  EXPECT_DOUBLE_EQ(sut.xt(), 0.05);
  EXPECT_DOUBLE_EQ(sut.ap(), 0.03);
  EXPECT_DOUBLE_EQ(sut.tauApFast(), 10);
  EXPECT_DOUBLE_EQ(sut.tauApSlow(), 80);
  EXPECT_DOUBLE_EQ(sut.apSlowFraction(), 0.8);
  EXPECT_DOUBLE_EQ(sut.ccgv(), 0.05);
  EXPECT_DOUBLE_EQ(sut.snrdB(), 30);
  EXPECT_DOUBLE_EQ(sut.gain(), 1);
  EXPECT_TRUE(sut.hasDcr());
  EXPECT_TRUE(sut.hasXt());
  EXPECT_TRUE(sut.hasAp());
  EXPECT_FALSE(sut.hasSlowComponent());
}

TEST_F(TestSiPMProperties, SetPropertyWithSetter) {
  SiPMProperties lsut = sut;
  for (int i = 0; i < 1000; ++i) {
    double dcr = rng.Rand() * 100e3;
    double xt = rng.Rand();
    double ap = rng.Rand();
    lsut.setDcr(dcr);
    lsut.setXt(xt);
    lsut.setAp(ap);
    EXPECT_DOUBLE_EQ(lsut.dcr(), dcr);
    EXPECT_DOUBLE_EQ(lsut.xt(), xt);
    EXPECT_DOUBLE_EQ(lsut.ap(), ap);
  }
}

TEST_F(TestSiPMProperties, SetPropertyWithString) {
  SiPMProperties lsut = sut;
  for (int i = 0; i < 1000; ++i) {
    double dcr = rng.Rand() * 100e3;
    double xt = rng.Rand();
    double ap = rng.Rand();
    lsut.setProperty("Dcr", dcr);
    lsut.setProperty("Xt", xt);
    lsut.setProperty("Ap", ap);
    EXPECT_DOUBLE_EQ(lsut.dcr(), dcr);
    EXPECT_DOUBLE_EQ(lsut.xt(), xt);
    EXPECT_DOUBLE_EQ(lsut.ap(), ap);
  }
}

TEST_F(TestSiPMProperties, SetHitDistribution) {
  SiPMProperties lsut = sut;
  lsut.setHitDistribution(SiPMProperties::HitDistribution::kUniform);
  EXPECT_TRUE(lsut.hitDistribution() == SiPMProperties::HitDistribution::kUniform);
  lsut.setHitDistribution(SiPMProperties::HitDistribution::kGaussian);
  EXPECT_TRUE(lsut.hitDistribution() == SiPMProperties::HitDistribution::kGaussian);
  lsut.setHitDistribution(SiPMProperties::HitDistribution::kCircle);
  EXPECT_TRUE(lsut.hitDistribution() == SiPMProperties::HitDistribution::kCircle);
}

TEST_F(TestSiPMProperties, SetHitPdeType) {
  SiPMProperties lsut = sut;
  lsut.setPdeType(SiPMProperties::PdeType::kNoPde);
  EXPECT_TRUE(lsut.pdeType() == SiPMProperties::PdeType::kNoPde);
  lsut.setPdeType(SiPMProperties::PdeType::kSpectrumPde);
  EXPECT_TRUE(lsut.pdeType() == SiPMProperties::PdeType::kSpectrumPde);
  lsut.setPdeType(SiPMProperties::PdeType::kSimplePde);
  EXPECT_TRUE(lsut.pdeType() == SiPMProperties::PdeType::kSimplePde);
  lsut.setPde(0.3);
  EXPECT_DOUBLE_EQ(lsut.pde(), 0.3);
}

TEST_F(TestSiPMProperties, SetPdeSpectrum) {
  const int N = 20;
  SiPMProperties lsut = sut;
  lsut.setPdeType(SiPMProperties::PdeType::kSpectrumPde);
  std::map<double, double> pde;
  for (int i = 0; i < N; ++i) {
    pde[i * 50] = rng.Rand();
  }
  lsut.setPdeSpectrum(pde);
  std::map<double, double> pde_return = lsut.pdeSpectrum();
  for (int i = 0; i < N; ++i) {
    EXPECT_DOUBLE_EQ(pde[i * 50], pde_return[i * 50]);
  }
}
