// Precomputes the deposited-energy spectra of every gamma source in
// data/nuclides for every scintillator material and size preset in
// data/scintillators.json. Run at build time; the GUI then only samples
// energies from these spectra.
//
// Usage: sipm_gui_spectra <data dir> <output dir> [detected events per spectrum]

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "FileIO.h"
#include "Scintillator.h"

using namespace sipmgui;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "Usage: %s <data dir> <output dir> [detected events]\n", argv[0]);
    return 1;
  }
  const fs::path data(argv[1]), out(argv[2]);
  const uint64_t nDetected = argc > 3 ? std::stoull(argv[3]) : 200000;

  ScintillatorDb db;
  const std::string err = loadScintillatorDb((data / "scintillators.json").string(), db);
  if (!err.empty()) {
    std::fprintf(stderr, "%s\n", err.c_str());
    return 1;
  }
  struct Source {
    std::string name;
    std::vector<GammaLine> lines;
  };
  std::vector<Source> sources;
  for (const auto& entry : fs::directory_iterator(data / "nuclides")) {
    if (entry.path().extension() != ".csv") {
      continue;
    }
    std::vector<double> e, in;
    if (!loadLinesCsv(entry.path().string(), e, in).empty()) {
      continue;
    }
    Source s{entry.path().stem().string(), {}};
    for (size_t i = 0; i < e.size(); ++i) {
      s.lines.push_back({e[i], in[i]});
    }
    sources.push_back(std::move(s));
  }

  struct Job {
    size_t m, s, src;
  };
  std::vector<Job> jobs;
  for (size_t m = 0; m < db.materials.size(); ++m) {
    for (size_t s = 0; s < db.sizes.size(); ++s) {
      for (size_t k = 0; k < sources.size(); ++k) {
        jobs.push_back({m, s, k});
      }
    }
  }
  fs::create_directories(out);
  const auto t0 = std::chrono::steady_clock::now();
  std::atomic<size_t> next{0};
  std::atomic<bool> failed{false};
  auto work = [&] {
    for (size_t j = next++; j < jobs.size(); j = next++) {
      const ScintMaterial& mat = db.materials[jobs[j].m];
      const CrystalSize& size = db.sizes[jobs[j].s];
      const Source& src = sources[jobs[j].src];
      const SpectrumResult r =
        simulateSpectrum(mat, size, src.lines, (data / "xcom").string(), nDetected, 0x5151 + j * 7919);
      if (!r.error.empty()) {
        std::fprintf(stderr, "%s %s %s: %s\n", mat.id.c_str(), size.id.c_str(), src.name.c_str(), r.error.c_str());
        failed = true;
        continue;
      }
      std::ofstream f(out / spectrumFileName(mat.id, size.id, src.name));
      f << "# " << src.name << " in " << mat.name << ", " << size.name << ": deposited energy, 1 keV bins\n";
      f << "# Monte Carlo with NIST XCOM cross sections; intrinsic FWHM " << mat.resolution662 << "% at 662 keV\n";
      f << "# decays " << r.decays << ", detected " << r.detected << " (efficiency "
        << static_cast<double>(r.detected) / r.decays << ")\n";
      for (size_t i = 0; i < r.counts.size(); ++i) {
        f << r.counts[i] << (i + 1 == r.counts.size() || i % 32 == 31 ? '\n' : ',');
      }
    }
  };
  std::vector<std::thread> pool;
  for (unsigned i = 0; i < std::max(1u, std::thread::hardware_concurrency()); ++i) {
    pool.emplace_back(work);
  }
  for (auto& t : pool) {
    t.join();
  }
  const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("%zu spectra (%zu materials x %zu sizes x %zu sources) in %.1f s\n", jobs.size(), db.materials.size(),
              db.sizes.size(), sources.size(), dt);
  if (failed) {
    return 1;
  }
  std::ofstream(out / "stamp") << "ok\n";
  return 0;
}
