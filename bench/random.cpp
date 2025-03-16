#include "SiPM.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdlib>
#include <vector>

class BenchmarkRandom : public benchmark::Fixture {
public:
  sipm::SiPMRng::Xorshift256plus m_rng;
  sipm::SiPMRandom m_random;

  void SetUp(const ::benchmark::State& state) {
    m_rng.seed();
    // Heat up cache
    for (size_t i = 0; i < 1024; i++) {
      m_rng();
      m_random.Rand();
    }
  }

  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(BenchmarkRandom, Rng)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_rng());
  }
}

BENCHMARK_F(BenchmarkRandom, SingleRandomDouble)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.Rand());
  }
}

BENCHMARK_F(BenchmarkRandom, SingleRandomFloat)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.Rand<float>());
  }
}

BENCHMARK_F(BenchmarkRandom, PairRandomFloat)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.RandF2());
  }
}

BENCHMARK_F(BenchmarkRandom, SingleGuassianDouble)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randGaussian(0, 1));
  }
}

BENCHMARK_F(BenchmarkRandom, SingleGuassianFloat)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randGaussianF(0, 1));
  }
}

BENCHMARK_F(BenchmarkRandom, SingleInteger)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randInteger(10));
  }
}

BENCHMARK_F(BenchmarkRandom, SingleExponential)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randExponential(10));
  }
}

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleRng)(benchmark::State& st) {
  for (auto _ : st) {
    uint64_t* x = (uint64_t*)aligned_alloc(64,st.range(0)*sizeof(uint64_t));
    m_rng.getRand(x,st.range(0));
    free(x);
  }
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleRng)->RangeMultiplier(2)->Range(8, 1 << 16)->Complexity(benchmark::oN);

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleRandomDouble)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.Rand(st.range(0)));
  }
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleRandomDouble)
  ->RangeMultiplier(2)
  ->Range(1, 1 << 12)
  ->Complexity(benchmark::oN);

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleRandomFloat)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.RandF(st.range(0)));
  }
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleRandomFloat)
  ->RangeMultiplier(2)
  ->Range(1, 1 << 12)
  ->Complexity(benchmark::oN);

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleInteger)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randInteger(10, st.range(0)));
  }
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleInteger)
  ->RangeMultiplier(2)
  ->Range(1, 1 << 12)
  ->Complexity(benchmark::oN);

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleGaussianDouble)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randGaussian(0, 1, st.range(0)));
  }
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleGaussianDouble)
  ->RangeMultiplier(2)
  ->Range(1, 1 << 12)
  ->Complexity(benchmark::oN);

BENCHMARK_DEFINE_F(BenchmarkRandom, MultipleGaussianFloat)(benchmark::State& st) {
  for (auto _ : st) {
    benchmark::DoNotOptimize(m_random.randGaussianF(0, 1, st.range(0)));
  }
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(BenchmarkRandom, MultipleGaussianFloat)
  ->RangeMultiplier(2)
  ->Range(1, 1 << 12)
  ->Complexity(benchmark::oN);

// Run the benchmark
BENCHMARK_MAIN();
