#include "SiPM.h"
#include <gtest/gtest.h>
#include <stdint.h>

#include <iostream>

using namespace sipm;

struct TestSiPMSensor : public ::testing::Test {
  SiPMSensor sut;
  SiPMRandom rng;
};

TEST_F(TestSiPMSensor, Constructor) { SiPMSensor sensor = SiPMSensor(); }

TEST_F(TestSiPMSensor, AddPhoton) {
  static constexpr int N = 100000;
  for (int i = 0; i < N; ++i) {
    sut.resetState();
    const double t = rng.randGaussian(100, 0.1);
    sut.addPhoton(t);
  }
}

TEST_F(TestSiPMSensor, AddPhotons) {
  static constexpr int N = 100000;
  for (int i = 0; i < N; ++i) {
    sut.resetState();
    int n = rng.randInteger(100) + 1;
    // n should be > 0
    const std::vector<double> t = rng.randGaussian(100, 0.1, n);
    sut.addPhotons(t);
  }
}

TEST_F(TestSiPMSensor, AddPhotonWlen) {
  static constexpr int N = 100000;
  for (int i = 0; i < N; ++i) {
    sut.resetState();
    const double t = rng.randGaussian(100, 0.1);
    const double w = rng.randGaussian(450, 20);
    sut.addPhoton(t, w);
  }
}

TEST_F(TestSiPMSensor, AddPhotonsWlen) {
  static constexpr int N = 100000;
  sut.resetState();
  for (int i = 0; i < N; ++i) {
    sut.resetState();
    int n = rng.randInteger(100) + 1;
    // n should be > 0
    const std::vector<double> t = rng.randGaussian(100, 0.2, n);
    const std::vector<double> w = rng.randGaussian(450, 20, n);
    sut.addPhotons(t, w);
  }
}

TEST_F(TestSiPMSensor, AddDcr) {
  static constexpr int N = 1000000;
  int ndcr = 0;
  SiPMSensor sensor;
  sensor.rng().rng().seed();
  for (int i = 0; i < N; ++i) {
    sensor.resetState();
    sensor.runEvent();
    ndcr += sensor.debug().nDcr;
  }
  const double rate = 1e9 * ((double)ndcr / N / sensor.properties().signalLength());
  EXPECT_GE(rate, sensor.properties().dcr() * 0.95);
  EXPECT_LE(rate, sensor.properties().dcr() * 1.05);
}

TEST_F(TestSiPMSensor, SignalGeneration) {
  static constexpr int N = 25;
  static constexpr int R = 10000;
  SiPMSensor lsut = SiPMSensor();
  // Almost no noise
  lsut.properties().setXtOff();
  lsut.properties().setDcrOff();
  lsut.properties().setApOff();
  lsut.properties().setSnr(40);

  for (int i = 1; i < N; ++i) {
    // generate i photons
    const std::vector<double> t = rng.randGaussian(10, 0.1, i);
    double avg_max = 0;
    for (int j = 0; j < R; ++j) {
      lsut.resetState();
      lsut.addPhotons(t);
      lsut.runEvent();
      avg_max += lsut.signal().peak(0, 20, 0);
    }
    avg_max /= R;
    EXPECT_GE(avg_max + 0.5, i);
    EXPECT_LE(avg_max - 0.5, i);
  }
}
