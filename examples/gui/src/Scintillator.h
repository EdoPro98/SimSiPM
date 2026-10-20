#ifndef SIPM_GUI_SCINTILLATOR_H
#define SIPM_GUI_SCINTILLATOR_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sipmgui {

/// Fixed properties of a scintillator material (data/scintillators.json).
struct ScintMaterial {
  std::string id, name;
  double density = 1;                                      // g/cm3
  std::vector<std::pair<std::string, double>> composition; // element symbol, atoms per formula unit
  double lightYield = 0;                                   // photons/keV
  double resolution662 = 0;                                // intrinsic FWHM at 662 keV [%], ~sqrt(E)
  double riseTime = 0;                                     // ns
  std::vector<std::pair<double, double>> decay;            // (tau ns, fraction)
  double emissionPeak = 420, emissionFwhm = 60;            // nm, Gaussian approximation
};

/// Crystal geometry preset. The source sits on the crystal axis.
struct CrystalSize {
  std::string id, name;
  bool cylinder = false;                      // box (width x width x length) or cylinder (diameter width, length)
  double width = 0, length = 0, distance = 0; // mm
};

struct ScintillatorDb {
  std::vector<ScintMaterial> materials;
  std::vector<CrystalSize> sizes;
};

/// Loads data/scintillators.json. Returns an empty string on success.
std::string loadScintillatorDb(const std::string& path, ScintillatorDb& db);

/// Photon emission line of a source: annihilation lines (511 keV) are
/// emitted as back-to-back pairs, `intensity` counting both photons.
struct GammaLine {
  double energy;    // keV
  double intensity; // photons per 100 decays
};

struct SpectrumResult {
  std::vector<double> counts; // deposited energy, 1 keV bins, intrinsic resolution applied
  uint64_t decays = 0;        // simulated decays
  uint64_t detected = 0;      // decays depositing energy
  std::string error;
};

/// Monte Carlo of source decays seen by a crystal: photons are emitted
/// isotropically from a point source and transported through the crystal
/// with NIST XCOM cross sections (data/xcom): photoelectric absorption with
/// K X-ray fluorescence and escape, Klein-Nishina Compton scattering and
/// pair production with annihilation photons. All photons of a decay are
/// tracked, so true coincidence summing is included. Electrons deposit their
/// energy locally; coherent scattering is neglected.
SpectrumResult simulateSpectrum(const ScintMaterial& material, const CrystalSize& size,
                                const std::vector<GammaLine>& lines, const std::string& xcomDir, uint64_t nDetected,
                                uint64_t seed);

/// File name of a precomputed spectrum
std::string spectrumFileName(const std::string& materialId, const std::string& sizeId, const std::string& source);

} // namespace sipmgui
#endif
