// Benchmarks quoted in the README: cost of a whole event for typical sizes.
//
//   Events/<size>/<mode>      size: Empty, 25 photons, 1000 photons
//                              mode: Full (hits + signal) or HitsOnly (runEvent(false))
//   PdeBench/<size>/<type>/<mode>  same sizes with no PDE, a flat PDE and a spectral PDE
//   Scan/<mode>/<N>           N = 1, 2, 4, ... 16384 photons; the counter "pe" is the mean number
//                              of hit cells (photoelectrons, crosstalk, afterpulses, dark counts)
//
// Default sensor (1 mm, 25 um cells, 500 ns window, DCR 200 kHz, XT 5%, AP 3%).
// Photon times follow a scintillation-like exponential decay (tau = 40 ns) from t = 10 ns.
//
//   ./events --benchmark_repetitions=7 --benchmark_report_aggregates_only=true

#include "../include/SiPM.h"
#include <algorithm>
#include <benchmark/benchmark.h>
#include <string>
#include <vector>

namespace {
// Photon sets cycled through, so branch prediction cannot learn one event (fewer for huge events)
int numSets(const int n) { return std::max(8, std::min(256, 2000000 / std::max(n, 1))); }

struct Photons {
  std::vector<std::vector<double>> times, wavelengths;
  Photons(const int n) : times(numSets(n)), wavelengths(numSets(n)) {
    sipm::SiPMRandom rng;
    for (size_t s = 0; s < times.size(); ++s) {
      times[s] = rng.randExponential(40.0, n);
      for (double& t : times[s]) t += 10.0;
      wavelengths[s] = rng.Rand(n);
      for (double& w : wavelengths[s]) w = 400 + 200 * w; // 400-600 nm
    }
  }
};

enum class Pde { kNone, kFlat, kSpectral };

sipm::SiPMProperties props(const Pde pde) {
  sipm::SiPMProperties p; // defaults
  switch (pde) {
    case Pde::kNone: break;
    case Pde::kFlat:
      p.setPde(0.42); // mean of the spectrum below over 400-600 nm
      break;
    case Pde::kSpectral:
      p.setPdeSpectrum({800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300},
                       {0.22, 0.30, 0.40, 0.45, 0.50, 0.50, 0.45, 0.35, 0.25, 0.15, 0.05});
      break;
  }
  return p;
}

void run(benchmark::State& st, const int nPhotons, const Pde pde, const bool signal) {
  static std::vector<Photons> cache;
  static std::vector<int> cacheN;
  size_t idx = 0;
  for (; idx < cacheN.size(); ++idx) if (cacheN[idx] == nPhotons) break;
  if (idx == cacheN.size()) { cache.emplace_back(nPhotons); cacheN.push_back(nPhotons); }
  const Photons& ph = cache[idx];

  sipm::SiPMSensor sensor(props(pde));
  for (size_t i = 0; i < 2000; ++i) { // warm-up
    sensor.resetState();
    sensor.addPhotons(ph.times[i % ph.times.size()]);
    sensor.runEvent(signal);
  }
  size_t k = 0;
  double hitCells = 0;
  for (auto _ : st) {
    sensor.resetState();
    if (nPhotons > 0) {
      if (pde == Pde::kSpectral) sensor.addPhotons(ph.times[k], ph.wavelengths[k]);
      else sensor.addPhotons(ph.times[k]);
    }
    sensor.runEvent(signal);
    benchmark::ClobberMemory();
    hitCells += sensor.debug().nPhotoelectrons;
    k = (k + 1) % ph.times.size();
  }
  st.counters["pe"] = benchmark::Counter(hitCells, benchmark::Counter::kAvgIterations);
}

void Events(benchmark::State& st, int n, bool signal) { run(st, n, Pde::kNone, signal); }
void PdeBench(benchmark::State& st, int n, Pde pde, bool signal) { run(st, n, pde, signal); }
void Scan(benchmark::State& st, bool signal) { run(st, static_cast<int>(st.range(0)), Pde::kNone, signal); }
} // namespace

#define EVENTS(label, n)                                                  \
  BENCHMARK_CAPTURE(Events, label / Full, n, true);                       \
  BENCHMARK_CAPTURE(Events, label / HitsOnly, n, false);
EVENTS(Empty, 0)
EVENTS(Photons25, 25)
EVENTS(Photons1000, 1000)

#define PDES(label, n)                                                    \
  BENCHMARK_CAPTURE(PdeBench, label / NoPde / Full, n, Pde::kNone, true);        \
  BENCHMARK_CAPTURE(PdeBench, label / FlatPde / Full, n, Pde::kFlat, true);      \
  BENCHMARK_CAPTURE(PdeBench, label / SpectralPde / Full, n, Pde::kSpectral, true); \
  BENCHMARK_CAPTURE(PdeBench, label / NoPde / HitsOnly, n, Pde::kNone, false);   \
  BENCHMARK_CAPTURE(PdeBench, label / FlatPde / HitsOnly, n, Pde::kFlat, false); \
  BENCHMARK_CAPTURE(PdeBench, label / SpectralPde / HitsOnly, n, Pde::kSpectral, false);
PDES(Photons25, 25)
PDES(Photons1000, 1000)

BENCHMARK_CAPTURE(Scan, Full, true)->RangeMultiplier(2)->Range(1, 1 << 14);
BENCHMARK_CAPTURE(Scan, HitsOnly, false)->RangeMultiplier(2)->Range(1, 1 << 14);

BENCHMARK_MAIN();
