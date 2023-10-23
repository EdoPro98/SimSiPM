#include "../include/SiPM.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <vector>

class BenchmarkSensor : public benchmark::Fixture {
public:
  sipm::SiPMSensor m_sensor;
  sipm::SiPMRandom m_rng;
  void SetUp(const benchmark::State& state) {
    for (int i = 0; i < 1 << 12; ++i) {
      m_sensor.resetState();
      m_sensor.addPhotons({10, 10, 10, 10});
      m_sensor.runEvent();
      const auto signal = m_sensor.signal();
      const auto integral = signal.integral(1, 250, 0.5);
    }
  }

  void TearDown(const benchmark::State& state) {}
};

BENCHMARK_F(BenchmarkSensor, DefaultNoLightSim)(benchmark::State& st) {
  m_sensor.setProperties(sipm::SiPMProperties());
  for (auto _ : st) {
    m_sensor.resetState();
    m_sensor.runEvent();
  }
}

BENCHMARK_F(BenchmarkSensor, DefaultNoNoiseNoLightSim)(benchmark::State& st) {
  m_sensor.properties().setDcrOff();
  m_sensor.properties().setApOff();
  m_sensor.properties().setXtOff();
  for (auto _ : st) {
    m_sensor.resetState();
    m_sensor.runEvent();
  }
}

BENCHMARK_DEFINE_F(BenchmarkSensor, DefaultLightSim)(benchmark::State& st) {
  m_sensor.setProperties(sipm::SiPMProperties());
  for (auto _ : st) {
    st.PauseTiming();
    const std::vector<double> t = m_rng.randGaussian(10, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
}
BENCHMARK_REGISTER_F(BenchmarkSensor, DefaultLightSim)->RangeMultiplier(2)->Range(1, 1 << 12);

BENCHMARK_DEFINE_F(BenchmarkSensor, DefaultNoNoiseLightSim)(benchmark::State& st) {
  m_sensor.setProperties(sipm::SiPMProperties());
  m_sensor.properties().setDcrOff();
  m_sensor.properties().setApOff();
  m_sensor.properties().setXtOff();
  for (auto _ : st) {
    st.PauseTiming();
    const std::vector<double> t = m_rng.randGaussian(10, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
}
BENCHMARK_REGISTER_F(BenchmarkSensor, DefaultNoNoiseLightSim)->RangeMultiplier(2)->Range(1, 1 << 12);

BENCHMARK_DEFINE_F(BenchmarkSensor, DefaultWlenLightSim)(benchmark::State& st) {
  m_sensor.setProperties(sipm::SiPMProperties());
  const std::vector<double> wlen = {300, 400, 500, 600, 700};
  const std::vector<double> pde = {0.1, 0.3, 0.2, 0.1};
  m_sensor.properties().setPdeSpectrum(wlen, pde);
  for (auto _ : st) {
    st.PauseTiming();
    const std::vector<double> t = m_rng.randGaussian(10, 0.1, st.range(0));
    const std::vector<double> w = m_rng.randGaussian(550, 10, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t, w);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
}
BENCHMARK_REGISTER_F(BenchmarkSensor, DefaultWlenLightSim)->RangeMultiplier(2)->Range(1, 1 << 12);

BENCHMARK_DEFINE_F(BenchmarkSensor, DefaultFullEvent)(benchmark::State& st) {
  m_sensor.setProperties(sipm::SiPMProperties());

  double integral, peak, tot;
  for (auto _ : st) {
    st.PauseTiming();
    const std::vector<double> t = m_rng.randGaussian(10, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    const sipm::SiPMAnalogSignal signal = m_sensor.signal();
    benchmark::DoNotOptimize(integral = signal.integral(5, 250, 0.5));
    benchmark::DoNotOptimize(peak = signal.peak(5, 250, 0.5));
    benchmark::DoNotOptimize(tot = signal.tot(5, 250, 0.5));
  }
}
BENCHMARK_REGISTER_F(BenchmarkSensor, DefaultFullEvent)->RangeMultiplier(2)->Range(1, 1 << 12);
BENCHMARK_MAIN();
