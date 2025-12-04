#include "sipm/SiPM.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <sipm/SiPMTypes.h>

class BenchmarkRandom : public benchmark::Fixture {
public:
  sipm::SiPMRng::Xorshift256plus m_rng;

  void SetUp(const ::benchmark::State& state) {
    m_rng.seed();
    // Heat up cache
    uint64_t* x = (uint64_t*)aligned_alloc(64, 8192 * sizeof(uint64_t));
    for (size_t i = 0; i < 1024; i++) {
      m_rng();
      m_rng.getRand(x, 8192);
    }
    free(x);
  }
};

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleRng)(benchmark::State& st) {
  const uint64_t n = st.range(0);
  const uint64_t bytes = (8 * n + 63) & ~63;
  uint64_t* x = (uint64_t*)sipm::sipmAlloc(bytes);
  for (auto _ : st) {
    m_rng.getRand(x, n);
    benchmark::DoNotOptimize(x);
    benchmark::ClobberMemory();
    st.SetBytesProcessed(uint64_t(st.iterations()) * uint64_t(bytes));
  }
  sipm::sipmFree(x);
  st.counters["Bytes"] = benchmark::Counter(bytes, benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
  st.counters["Rate"] = benchmark::Counter(bytes, benchmark::Counter::kIsRate, benchmark::Counter::OneK::kIs1024);
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleRng)
  ->DenseRange(1ul << 27, 1ul << 31, 1 << 26)
  ->ReportAggregatesOnly(true)
  ->Repetitions(16);

// Run the benchmark
BENCHMARK_MAIN();
