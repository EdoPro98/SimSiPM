// Benchmarks for the SiPM RNG subsystem.
//
// Covers three layers of the random generation stack:
//   1. Core PRNG  — xoshiro256+ ring-buffer mechanics (buffer hit vs refill)
//   2. Batched raw fills — getRand(uint64_t*/uint32_t*, n), AVX512 vs scalar
//   3. Distribution wrappers — uniform, Gaussian, exponential, integer, Poisson
//
// Every benchmark reports items/sec and, where memory bandwidth is relevant,
// bytes/sec.  Parametric benchmarks also report empirical complexity so you
// can verify that the implementation is truly O(N).
//
// Instruction-level profiling workflow:
//   # hardware PMU counters (Linux, requires libpfm4 + benchmark built with it)
//   ./random --benchmark_perf_counters=INSTRUCTIONS,CYCLES,BRANCH-MISSES
//
//   # or via perf:
//   perf stat -e instructions,cycles,cache-misses,branch-misses ./random \
//     --benchmark_filter=Scalar
//   perf record -g ./random --benchmark_filter=VecGaussianDouble \
//     --benchmark_min_time=5s
//   perf report

#include "SiPM.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdlib>
#include <vector>

// ============================================================================
// Shared fixture — seeded with a fixed value for reproducibility.
// The warm-up loop ensures the ring buffer is hot before any timed iteration.
// ============================================================================

class RngFixture : public benchmark::Fixture {
public:
  sipm::SiPMRng::Xorshift256plus m_xrng;
  sipm::SiPMRandom               m_rng;

  void SetUp(const benchmark::State&) override {
    m_xrng.seed(0xDEADBEEFCAFEBABEULL);
    m_rng.seed(0xDEADBEEFCAFEBABEULL);
    // Heat the ring buffer (65536 entries) and branch predictor
    for (int i = 0; i < 2048; ++i) {
      benchmark::DoNotOptimize(m_xrng());
      benchmark::DoNotOptimize(m_rng.Rand<double>());
    }
  }
};

// ============================================================================
// 1. Core PRNG — ring-buffer mechanics
// ============================================================================

// Hot-buffer fetch: index < N branch always taken, just an indexed load + inc.
BENCHMARK_F(RngFixture, XorshiftBufferedFetch)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_xrng());
  st.SetItemsProcessed(st.iterations());
  st.SetBytesProcessed(st.iterations() * sizeof(uint64_t));
}

// Buffer-refill cost: re-seed before every call to force index == N,
// then measure the first call which fills all 65536 ring-buffer slots.
BENCHMARK_F(RngFixture, XorshiftBufferRefill)(benchmark::State& st) {
  for (auto _ : st) {
    st.PauseTiming();
    m_xrng.seed(42ULL);          // sets index = N → triggers a full refill
    st.ResumeTiming();
    benchmark::DoNotOptimize(m_xrng());
  }
  // 65536 × 8 bytes generated per iteration
  st.SetBytesProcessed(st.iterations() * (1 << 16) * sizeof(uint64_t));
}

// ============================================================================
// 2. Batched raw fills — getRand(T*, n)
//
// These exercise the inner PRNG loop directly, bypassing the ring buffer.
// On AVX512 machines the uint64_t overload generates 8 values per SIMD step;
// the uint32_t overload generates 16 values per step.
// ============================================================================

// Batched uint64_t generation — one xoshiro256+ step per value (scalar) or
// eight values per step (AVX512).
BENCHMARK_DEFINE_F(RngFixture, GetRandU64)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  void* raw = aligned_alloc(64, static_cast<size_t>(n) * sizeof(uint64_t));
  auto* buf = static_cast<uint64_t*>(raw);
  for (auto _ : st) {
    m_xrng.getRand(buf, n);
    benchmark::ClobberMemory();
  }
  free(raw);
  st.SetItemsProcessed(st.iterations() * n);
  st.SetBytesProcessed(st.iterations() * n * static_cast<int64_t>(sizeof(uint64_t)));
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, GetRandU64)
    ->RangeMultiplier(2)->Range(8, 1 << 17)->Complexity(benchmark::oN);

// Batched uint32_t generation — scalar path produces two u32 per u64 step;
// AVX512 path produces 16 u32 per 512-bit store.
BENCHMARK_DEFINE_F(RngFixture, GetRandU32)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  void* raw = aligned_alloc(64, static_cast<size_t>(n) * sizeof(uint32_t));
  auto* buf = static_cast<uint32_t*>(raw);
  for (auto _ : st) {
    m_xrng.getRand(buf, n);
    benchmark::ClobberMemory();
  }
  free(raw);
  st.SetItemsProcessed(st.iterations() * n);
  st.SetBytesProcessed(st.iterations() * n * static_cast<int64_t>(sizeof(uint32_t)));
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, GetRandU32)
    ->RangeMultiplier(2)->Range(16, 1 << 17)->Complexity(benchmark::oN);

// ============================================================================
// 3. Scalar uniform conversions
// ============================================================================

// Double in [0,1): one buffer fetch + right-shift + fused-mul.
BENCHMARK_F(RngFixture, ScalarRandDouble)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.Rand<double>());
  st.SetItemsProcessed(st.iterations());
}

// Float in [0,1): same but uses top 24 bits only.
BENCHMARK_F(RngFixture, ScalarRandFloat)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.Rand<float>());
  st.SetItemsProcessed(st.iterations());
}

// Two floats from a single 64-bit word: lo/hi 32-bit split.
// Throughput should be ~2× ScalarRandFloat at the same ns/call cost.
BENCHMARK_F(RngFixture, ScalarRandF2)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.RandF2());
  st.SetItemsProcessed(st.iterations() * 2);  // 2 values per call
}

// ============================================================================
// 4. Scalar derived distributions
// ============================================================================

// Gaussian (double) — Marsaglia polar: ~2.5 Rand(), one log, div and sqrt.
// Rejection loop accepts 78.5% of points.
BENCHMARK_F(RngFixture, ScalarGaussianDouble)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randGaussian(0.0, 1.0));
  st.SetItemsProcessed(st.iterations());
}

// Gaussian (float) — the double method narrowed to float.
BENCHMARK_F(RngFixture, ScalarGaussianFloat)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randGaussianF(0.0f, 1.0f));
  st.SetItemsProcessed(st.iterations());
}

// Exponential (double) — -log(U)*mu: one log() call per sample.
BENCHMARK_F(RngFixture, ScalarExponentialDouble)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randExponential(10.0));
  st.SetItemsProcessed(st.iterations());
}

// Exponential (float) — -logf(U)*mu.
BENCHMARK_F(RngFixture, ScalarExponentialFloat)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randExponentialF(10.0f));
  st.SetItemsProcessed(st.iterations());
}

// Integer in [0, max): high 32 bits of u64 multiplied, then shifted.
BENCHMARK_F(RngFixture, ScalarInteger)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randInteger(40));
  st.SetItemsProcessed(st.iterations());
}

// Two integers from a single 64-bit word — lo/hi split + multiply each.
// At same ns/call cost this should deliver 2× throughput vs ScalarInteger.
BENCHMARK_F(RngFixture, ScalarIntegerPair)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randInteger2(40));
  st.SetItemsProcessed(st.iterations() * 2);
}

// Poisson (mu=2) — Knuth sequential product; ~exp(2)≈7 multiplies expected.
BENCHMARK_F(RngFixture, PoissonSmallMu)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randPoisson(2.0));
  st.SetItemsProcessed(st.iterations());
}

// Poisson (mu=25) — Knuth algorithm, ~25 Rand() calls per sample.
BENCHMARK_F(RngFixture, PoissonMediumMu)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randPoisson(25.0));
  st.SetItemsProcessed(st.iterations());
}

// Poisson (mu=100) — Gaussian approximation (> 50 threshold); O(1) calls.
// Should be significantly faster than PoissonMediumMu despite larger mu.
BENCHMARK_F(RngFixture, PoissonLargeMu)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randPoisson(100.0));
  st.SetItemsProcessed(st.iterations());
}

// ============================================================================
// 5. Vectorised batch generation — allocation cost included
//
// Each call allocates a std::vector<T>(n) and fills it.  The std::vector
// allocation overhead is visible at small N and negligible at large N.
// Complexity analysis separates allocation cost from generation cost.
// ============================================================================

// Batched double: getRand u64 → in-place bit-manipulation to double.
BENCHMARK_DEFINE_F(RngFixture, VecRandDouble)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.Rand(n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetBytesProcessed(st.iterations() * n * static_cast<int64_t>(sizeof(double)));
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecRandDouble)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

// Batched float: getRand u32 → in-place bit-manipulation to float.
BENCHMARK_DEFINE_F(RngFixture, VecRandFloat)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.RandF(n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetBytesProcessed(st.iterations() * n * static_cast<int64_t>(sizeof(float)));
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecRandFloat)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

// Batched double Gaussian: Box-Muller pairs (1 sqrt + 1 sincos per 2 values).
// At large N the log+sincos dominates; compare ns/item to ScalarGaussianDouble.
BENCHMARK_DEFINE_F(RngFixture, VecGaussianDouble)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randGaussian(0.0, 1.0, n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecGaussianDouble)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

// Batched float Gaussian: Box-Muller in single precision (sqrtf + sincosf).
BENCHMARK_DEFINE_F(RngFixture, VecGaussianFloat)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randGaussianF(0.0f, 1.0f, n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecGaussianFloat)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

// Batched double exponential: one log() per element; bandwidth-bound at large N.
BENCHMARK_DEFINE_F(RngFixture, VecExponentialDouble)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randExponential(10.0, n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetBytesProcessed(st.iterations() * n * static_cast<int64_t>(sizeof(double)));
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecExponentialDouble)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

// Batched float exponential: one logf() per element.
BENCHMARK_DEFINE_F(RngFixture, VecExponentialFloat)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randExponentialF(10.0f, n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetBytesProcessed(st.iterations() * n * static_cast<int64_t>(sizeof(float)));
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecExponentialFloat)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

// Batched integer in [0, max): multiply-shift loop; compare to GetRandU32.
BENCHMARK_DEFINE_F(RngFixture, VecInteger)(benchmark::State& st) {
  const int32_t n = static_cast<int32_t>(st.range(0));
  for (auto _ : st)
    benchmark::DoNotOptimize(m_rng.randInteger(40, n));
  st.SetItemsProcessed(st.iterations() * n);
  st.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(RngFixture, VecInteger)
    ->RangeMultiplier(2)->Range(1, 1 << 14)->Complexity(benchmark::oN);

BENCHMARK_MAIN();
