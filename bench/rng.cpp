#include <benchmark/benchmark.h>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <ostream>

#include "SiPMRandom.h"
#include "SiPMTypes.h"

class BenchmarkRng : public benchmark::Fixture {
public:
  sipm::SiPMRng::Xorshift256plus m_rng;

  void SetUp(const ::benchmark::State& state) {
    m_rng.seed();
    // Heat up cache
    for (size_t i = 0; i < 1 << 27; i++) {
      const uint64_t x = m_rng();
    }
  }
};

BENCHMARK_DEFINE_F(BenchmarkRng, SingleRng)(benchmark::State& st) {
  for (auto _ : st) {
    uint64_t x;
    benchmark::DoNotOptimize(x = m_rng());
  }
}
BENCHMARK_REGISTER_F(BenchmarkRng, SingleRng);

BENCHMARK_DEFINE_F(BenchmarkRng, MultipleRng)(benchmark::State& st) {
  const uint64_t n = st.range(0);
  uint64_t* x = (uint64_t*)sipm::sipmAlloc(n * sizeof(uint64_t));
  for (auto _ : st) {
    m_rng.getRand(x, n);
    benchmark::DoNotOptimize(x);
    benchmark::ClobberMemory();
  }
  st.SetBytesProcessed(uint64_t(st.iterations()) * uint64_t(n * sizeof(uint64_t)));
  st.counters["Bytes"] = benchmark::Counter(n * 8, benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
  sipm::sipmFree(x);
}
BENCHMARK_REGISTER_F(BenchmarkRng, MultipleRng)
  ->DenseRange(1 << 26, (1ul << 31) + (1ull << 26), 1 << 26)
  ->ReportAggregatesOnly(true)
  ->Iterations(64)
  ->Repetitions(8);

// Run the benchmark
BENCHMARK_MAIN();
