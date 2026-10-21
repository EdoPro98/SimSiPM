// Benchmarks for the SiPM sensor simulation.
//
// Organised in three groups:
//
//   A. Phase isolation
//      Each benchmark disables all effects except the one under study so you
//      can measure the raw cost of each runEvent() sub-step.
//
//      addDcrEvents           → DCROnly
//      addPhotoelectrons      → PEGeneration{NoPDE,SimplePDE,SpectrumPDE}
//      addCorrelatedNoise     → CorrelatedNoise{XT,AP,XTAndAP}
//      calculateSignalAmplitudes + generateSignal are always active once hits
//      exist; their cost grows with photon count in the later benchmarks.
//
//   B. Algorithmic comparisons
//      Hit distribution (uniform / circle / gaussian)
//      PDE type (kNoPde / kSimplePde / kSpectrumPde)
//      Sensor geometry (pitch / size)
//      Signal length impact on waveform generation
//
//   C. Signal feature extraction
//      Measures integral / peak / tot / toa / top on a fixed pre-generated
//      waveform.  Shows the memory-bandwidth cost of scanning the sample array.
//
// Instruction-level profiling:
//   perf stat -e instructions,cycles,cache-misses,branch-misses \
//     ./sensor --benchmark_filter=FullSimulation
//   perf record -g ./sensor --benchmark_filter=GenerateSignal \
//     --benchmark_min_time=5s
//   perf report

#include "../include/SiPM.h"
#include "SiPMProperties.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// Property helpers
// ============================================================================

static sipm::SiPMProperties baseProps() {
  sipm::SiPMProperties p;
  // Explicit defaults so benchmarks are self-documenting
  p.setSize(1);           // 1 mm
  p.setPitch(25);         // 25 μm → 40×40 = 1600 cells
  p.setSignalLength(500); // 500 ns window
  p.setSampling(1);       // 1 ns/sample
  return p;
}

// Minimal config: only the SNR baseline Gaussian is generated.
static sipm::SiPMProperties noNoiseProps() {
  auto p = baseProps();
  p.setDcrOff();
  p.setXtOff();
  p.setApOff();
  // PdeType stays kNoPde (default) → all photons accepted
  return p;
}

// ============================================================================
// A. PHASE ISOLATION
// ============================================================================

// ----------------------------------------------------------------------------
// Fixture shared by all simulation benchmarks.
// The warm-up in SetUp stabilises branch predictor and caches.
// ----------------------------------------------------------------------------

class SensorFixture : public benchmark::Fixture {
public:
  sipm::SiPMSensor m_sensor;
  sipm::SiPMRandom m_aux; // separate RNG for photon-time generation

  void SetUp(const benchmark::State&) override {
    m_sensor.setProperties(baseProps());
    for (int i = 0; i < 4096; ++i) {
      m_sensor.resetState();
      m_sensor.addPhotons({5.0, 10.0, 15.0, 20.0});
      m_sensor.runEvent();
    }
    m_sensor.resetState();
  }
};

// A1 — Baseline: only the electronic noise waveform is generated.
//      runEvent() always starts with randGaussianF(0, snr, nSignalPoints).
//      All other work is disabled.  This is the irreducible floor.
BENCHMARK_F(SensorFixture, BaselineNoiseOnly)(benchmark::State& st) {
  m_sensor.setProperties(noNoiseProps());
  for (auto _ : st) {
    m_sensor.resetState();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations());
}

// A2 — DCR only: exponential inter-arrival time, random cell selection.
//      Each Arg value is the DCR rate in Hz.
//      Expected hits/event = rate × 500 ns:
//        200 kHz → 0.1 hits    10 MHz → 5 hits    100 MHz → 50 hits
BENCHMARK_DEFINE_F(SensorFixture, DCROnly)(benchmark::State& st) {
  auto p = noNoiseProps();
  const double dcrHz = static_cast<double>(st.range(0));
  p.setDcr(dcrHz);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    m_sensor.resetState();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations());
}
BENCHMARK_REGISTER_F(SensorFixture, DCROnly)
    ->Arg(200000)     // 200 kHz default
    ->Arg(1000000)    // 1 MHz
    ->Arg(10000000)   // 10 MHz
    ->Arg(100000000); // 100 MHz

// A3 — Photoelectron generation, kNoPde: every photon becomes a hit.
//      Measures hitCell() + SiPMHit allocation cost.
BENCHMARK_DEFINE_F(SensorFixture, PEGenerationNoPDE)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setPdeType(sipm::SiPMProperties::PdeType::kNoPde);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, PEGenerationNoPDE)
    ->RangeMultiplier(2)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// A4 — Photoelectron generation, kSimplePde (30% PDE): each photon draws one
//      Rand() for the PDE test; ~70% are discarded without allocating a hit.
BENCHMARK_DEFINE_F(SensorFixture, PEGenerationSimplePDE)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setPde(0.30);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, PEGenerationSimplePDE)
    ->RangeMultiplier(2)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// A5 — Photoelectron generation, kSpectrumPde: each photon looks up PDE via
//      std::map binary search (log(n_wavelength_points) per photon).
BENCHMARK_DEFINE_F(SensorFixture, PEGenerationSpectrumPDE)(benchmark::State& st) {
  auto p = noNoiseProps();
  const std::vector<double> wlen = {300, 400, 500, 600, 700};
  const std::vector<double> pde  = {0.10, 0.30, 0.35, 0.20, 0.10};
  p.setPdeSpectrum(wlen, pde);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    const auto w = m_aux.randGaussian(500.0, 50.0, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t, w);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, PEGenerationSpectrumPDE)
    ->RangeMultiplier(2)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// A6 — Optical crosstalk only (10% XT): each hit spawns a Poisson number of
//      XT hits in adjacent cells.  This drives recursive SiPMHit allocation.
BENCHMARK_DEFINE_F(SensorFixture, CorrelatedNoiseXT)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setXt(0.10);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, CorrelatedNoiseXT)
    ->RangeMultiplier(2)->Range(1, 1 << 10)->Complexity(benchmark::oN);

// A7 — Afterpulses only (5% AP): each hit may spawn one delayed hit in the
//      same cell; rejection-samples the exponential delay.
BENCHMARK_DEFINE_F(SensorFixture, CorrelatedNoiseAP)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setAp(0.05);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, CorrelatedNoiseAP)
    ->RangeMultiplier(2)->Range(1, 1 << 10)->Complexity(benchmark::oN);

// A8 — XT + AP combined (realistic defaults): compare to NoPDE to see the
//      overhead of correlated noise book-keeping.
BENCHMARK_DEFINE_F(SensorFixture, CorrelatedNoiseXTAndAP)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setXt(0.10);
  p.setAp(0.05);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, CorrelatedNoiseXTAndAP)
    ->RangeMultiplier(2)->Range(1, 1 << 10)->Complexity(benchmark::oN);

// ============================================================================
// B. ALGORITHMIC COMPARISONS
// ============================================================================

// B1 — Hit distribution: uniform (randInteger2, two 32-bit multiply-shifts).
BENCHMARK_DEFINE_F(SensorFixture, HitDistUniform)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setHitDistribution(sipm::SiPMProperties::HitDistribution::kUniform);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, HitDistUniform)
    ->RangeMultiplier(4)->Range(1, 1 << 10);

// B2 — Hit distribution: circle (rejection sampling inside unit circle).
//      90% inside → ~4/π ≈ 1.27 Rand() pairs per accept; 10% outside the
//      circle adds another rejection loop.
BENCHMARK_DEFINE_F(SensorFixture, HitDistCircle)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setHitDistribution(sipm::SiPMProperties::HitDistribution::kCircle);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, HitDistCircle)
    ->RangeMultiplier(4)->Range(1, 1 << 10);

// B3 — Hit distribution: Gaussian (two ACR Gaussian draws + rejection for
//      the 5% outside 1.64σ where it falls back to uniform).
BENCHMARK_DEFINE_F(SensorFixture, HitDistGaussian)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setHitDistribution(sipm::SiPMProperties::HitDistribution::kGaussian);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, HitDistGaussian)
    ->RangeMultiplier(4)->Range(1, 1 << 10);

// B4 — Geometry: small sensor (1 mm / 100 μm pitch → 10×10 = 100 cells).
//      High occupancy at large N: many photons share cells, increasing
//      sort work inside calculateSignalAmplitudes().
BENCHMARK_DEFINE_F(SensorFixture, GeomSmall100Cells)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setSize(1);
  p.setPitch(100); // 10×10 = 100 cells
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, GeomSmall100Cells)
    ->RangeMultiplier(4)->Range(1, 1 << 8)->Complexity(benchmark::oN);

// B5 — Geometry: default (1 mm / 25 μm → 40×40 = 1600 cells).
BENCHMARK_DEFINE_F(SensorFixture, GeomDefault1600Cells)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setSize(1);
  p.setPitch(25); // 40×40 = 1600 cells
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, GeomDefault1600Cells)
    ->RangeMultiplier(4)->Range(1, 1 << 10)->Complexity(benchmark::oN);

// B6 — Geometry: large (3 mm / 25 μm → 120×120 = 14400 cells).
//      Low occupancy even at large N; hash table stays sparse.
BENCHMARK_DEFINE_F(SensorFixture, GeomLarge14400Cells)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setSize(3);
  p.setPitch(25); // 120×120 = 14400 cells
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, GeomLarge14400Cells)
    ->RangeMultiplier(4)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// B7 — Signal length: 100 ns → 100 samples.
//      generateSignal() inner loop: n_hits × 100 float fma operations.
BENCHMARK_DEFINE_F(SensorFixture, SignalLen100ns)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setSignalLength(100);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, SignalLen100ns)
    ->RangeMultiplier(4)->Range(1, 1 << 10);

// B8 — Signal length: 500 ns → 500 samples (default).
BENCHMARK_DEFINE_F(SensorFixture, SignalLen500ns)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setSignalLength(500);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, SignalLen500ns)
    ->RangeMultiplier(4)->Range(1, 1 << 10);

// B9 — Signal length: 2000 ns → 2000 samples.
//      generateSignal() dominates; compare ns/hit to 100 ns to confirm O(L).
BENCHMARK_DEFINE_F(SensorFixture, SignalLen2000ns)(benchmark::State& st) {
  auto p = noNoiseProps();
  p.setSignalLength(2000);
  m_sensor.setProperties(p);

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, SignalLen2000ns)
    ->RangeMultiplier(4)->Range(1, 1 << 10);

// B10 — Full simulation: DCR + XT + AP + simple PDE (realistic production use).
//       Use this as the primary regression benchmark.
BENCHMARK_DEFINE_F(SensorFixture, FullSimulation)(benchmark::State& st) {
  m_sensor.setProperties(baseProps());

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, FullSimulation)
    ->RangeMultiplier(2)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// B10b — Same as FullSimulation but runEvent(false): hits only, no signal generation.
BENCHMARK_DEFINE_F(SensorFixture, HitsOnly)(benchmark::State& st) {
  m_sensor.setProperties(baseProps());

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent(false);
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, HitsOnly)
    ->RangeMultiplier(2)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// B11 — Full event + feature extraction: measures the complete pipeline a
//       user would run in practice (simulate → analyse waveform).
BENCHMARK_DEFINE_F(SensorFixture, FullEventWithAnalysis)(benchmark::State& st) {
  m_sensor.setProperties(baseProps());
  double integral, peak, tot;

  for (auto _ : st) {
    st.PauseTiming();
    const auto t = m_aux.randGaussian(10.0, 0.1, st.range(0));
    m_sensor.resetState();
    m_sensor.addPhotons(t);
    st.ResumeTiming();
    m_sensor.runEvent();
    const sipm::SiPMAnalogSignal sig = m_sensor.signal();
    benchmark::DoNotOptimize(integral = sig.integral(5.0, 250.0, 0.5));
    benchmark::DoNotOptimize(peak     = sig.peak(5.0, 250.0, 0.5));
    benchmark::DoNotOptimize(tot      = sig.tot(5.0, 250.0, 0.5));
  }
  st.SetItemsProcessed(st.iterations() * st.range(0));
  st.SetComplexityN(st.range(0));
}
BENCHMARK_REGISTER_F(SensorFixture, FullEventWithAnalysis)
    ->RangeMultiplier(2)->Range(1, 1 << 12)->Complexity(benchmark::oN);

// ============================================================================
// C. SIGNAL FEATURE EXTRACTION
//
// All benchmarks in this group operate on a fixed, pre-generated waveform
// so they measure only the analysis cost — not the simulation cost.
//
// A realistic 100-PE signal on a 500 ns window (500 float samples) is used.
// The waveform stays in L1/L2 cache throughout each benchmark; the reported
// ns/op reflects the scan cost over that hot buffer.
// ============================================================================

class SignalFixture : public benchmark::Fixture {
public:
  sipm::SiPMAnalogSignal m_signal;
  sipm::SiPMRandom       m_aux;

  void SetUp(const benchmark::State&) override {
    sipm::SiPMProperties p;
    p.setSignalLength(500);
    p.setSampling(1);
    sipm::SiPMSensor sensor(p);
    const auto t = m_aux.randGaussian(10.0, 0.5, 100);
    sensor.addPhotons(t);
    sensor.runEvent();
    m_signal = sensor.signal();
  }
};

BENCHMARK_F(SignalFixture, SigIntegral)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_signal.integral(5.0, 450.0, 0.5));
  st.SetItemsProcessed(st.iterations());
  // 500 float reads per scan
  st.SetBytesProcessed(st.iterations() * 500 * static_cast<int64_t>(sizeof(float)));
}

BENCHMARK_F(SignalFixture, SigPeak)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_signal.peak(5.0, 450.0, 0.5));
  st.SetItemsProcessed(st.iterations());
  st.SetBytesProcessed(st.iterations() * 500 * static_cast<int64_t>(sizeof(float)));
}

BENCHMARK_F(SignalFixture, SigTimeOverThreshold)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_signal.tot(5.0, 450.0, 0.5));
  st.SetItemsProcessed(st.iterations());
  st.SetBytesProcessed(st.iterations() * 500 * static_cast<int64_t>(sizeof(float)));
}

BENCHMARK_F(SignalFixture, SigTimeOfArrival)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_signal.toa(5.0, 450.0, 0.5));
  st.SetItemsProcessed(st.iterations());
  st.SetBytesProcessed(st.iterations() * 500 * static_cast<int64_t>(sizeof(float)));
}

BENCHMARK_F(SignalFixture, SigTimeOfPeak)(benchmark::State& st) {
  for (auto _ : st)
    benchmark::DoNotOptimize(m_signal.top(5.0, 450.0, 0.5));
  st.SetItemsProcessed(st.iterations());
  st.SetBytesProcessed(st.iterations() * 500 * static_cast<int64_t>(sizeof(float)));
}

// All five features extracted in one pass — measures whether consecutive
// array scans benefit from the still-hot cache lines left by the first scan.
BENCHMARK_F(SignalFixture, SigAllFeatures)(benchmark::State& st) {
  double integral, peak, tot, toa, top;
  for (auto _ : st) {
    benchmark::DoNotOptimize(integral = m_signal.integral(5.0, 450.0, 0.5));
    benchmark::DoNotOptimize(peak     = m_signal.peak(5.0, 450.0, 0.5));
    benchmark::DoNotOptimize(tot      = m_signal.tot(5.0, 450.0, 0.5));
    benchmark::DoNotOptimize(toa      = m_signal.toa(5.0, 450.0, 0.5));
    benchmark::DoNotOptimize(top      = m_signal.top(5.0, 450.0, 0.5));
    benchmark::ClobberMemory();
  }
  st.SetItemsProcessed(st.iterations() * 5);
  st.SetBytesProcessed(st.iterations() * 5 * 500 * static_cast<int64_t>(sizeof(float)));
}

// Integral scanning cost as a function of waveform length.
// Should be O(N) in the number of samples; compare bytes/sec to confirm
// memory-bandwidth saturation at large N.
BENCHMARK_DEFINE_F(SignalFixture, SigIntegralVsLength)(benchmark::State& st) {
  const int64_t len = st.range(0);
  sipm::SiPMProperties p;
  p.setSignalLength(static_cast<double>(len));
  p.setSampling(1);
  sipm::SiPMSensor sensor(p);
  const auto t = m_aux.randGaussian(10.0, 0.5, 100);
  sensor.addPhotons(t);
  sensor.runEvent();
  auto sig = sensor.signal();

  for (auto _ : st)
    benchmark::DoNotOptimize(sig.integral(5.0, static_cast<double>(len) - 10.0, 0.5));
  st.SetBytesProcessed(st.iterations() * len * static_cast<int64_t>(sizeof(float)));
  st.SetComplexityN(len);
}
BENCHMARK_REGISTER_F(SignalFixture, SigIntegralVsLength)
    ->RangeMultiplier(2)->Range(64, 1 << 14)->Complexity(benchmark::oN);

BENCHMARK_MAIN();
