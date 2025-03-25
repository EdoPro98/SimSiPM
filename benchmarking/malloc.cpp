#include "sipm/SiPM.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <vector>

class BenchmarkRandom : public benchmark::Fixture {
public:
  sipm::SiPMRng::Xorshift256plus m_rng;

  void SetUp(const ::benchmark::State& state) {
    m_rng.seed();
    // Heat up cache
    for (size_t i = 0; i < 1024; i++) {
      uint64_t* x;
      x = (uint64_t*)aligned_alloc(64, 8192);
      free(x);
    }
  }
};

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleRng)(benchmark::State& st) {
  constexpr uint64_t GB = 1024 * 1024 * 1024;
  const uint64_t n = 0.5 * GB * st.range(0);
  const uint64_t bytes = (n + 64 - 1) & ~(64 - 1);
  for (auto _ : st) {
    uint64_t* x;
    benchmark::DoNotOptimize(x = (uint64_t*)aligned_alloc(64, bytes));
    benchmark::ClobberMemory();
    free(x);
  }
  st.counters["Bytes"] = benchmark::Counter(bytes, benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
  st.counters["Rate"] = benchmark::Counter(bytes, benchmark::Counter::kIsRate, benchmark::Counter::OneK::kIs1024);
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleRng)->DenseRange(1, 48, 1)->Repetitions(16)->ReportAggregatesOnly(true);

// Run the benchmark
BENCHMARK_MAIN();
