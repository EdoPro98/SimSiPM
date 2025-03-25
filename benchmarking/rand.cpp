#include "sipm/SiPM.h"
#include <benchmark/benchmark.h>
#include <cstdint>

class BenchmarkRandom : public benchmark::Fixture {
public:
  sipm::SiPMRng::Xorshift256plus m_rng;

  void SetUp(const ::benchmark::State& state) {
    m_rng.seed();
    // Heat up cache
    for (size_t i = 0; i < 4096; i++) {
      const uint64_t x = m_rng();
    }
  }
};

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleRng)(benchmark::State& st) {
  for (auto _ : st) {
    uint64_t x;
    benchmark::DoNotOptimize(x = m_rng());
  }
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleRng)->Repetitions(64);

// Run the benchmark
BENCHMARK_MAIN();
