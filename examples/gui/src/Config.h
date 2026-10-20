#ifndef SIPM_GUI_CONFIG_H
#define SIPM_GUI_CONFIG_H

#include <string>
#include <vector>

#include "Scintillator.h"

namespace sipmgui {

enum class PdeMode : int { kNone, kConstant, kSpectrum };
enum class NPhotonMode : int { kFixed, kPoisson, kExponential, kFile, kGamma };
enum class TimeMode : int { kFixed, kUniform, kGaussian, kExponential, kScintillator };
enum class WavelengthMode : int { kFixed, kUniform, kGaussian };
enum class TimingMode : int { kIndependent, kPoisson, kPeriodic };

/// Complete simulation setup edited by the GUI and consumed (as an immutable
/// snapshot) by the generator and worker threads. Units are the ones shown
/// in the GUI (percent, kHz, ns, mm, um).
struct SimConfig {
  // Sensor
  double sizeMm = 1.0;
  double pitchUm = 25.0;
  int lightProfile = 0; // SiPMProperties::HitDistribution

  // Noise
  bool dcrOn = true;
  double dcrKHz = 200.0;
  bool xtOn = true;
  double xtPct = 5.0;
  bool dxtOn = false;
  double dxtPct = 5.0;
  double dxtTau = 15.0;
  bool apOn = true;
  double apPct = 3.0;
  double apTauFast = 10.0;
  double apTauSlow = 80.0;
  double apSlowPct = 50.0;
  double ccgvPct = 5.0;
  double snrDb = 30.0;

  // Signal
  double signalLength = 500.0;
  double sampling = 1.0;
  double riseTime = 1.0;
  double fallTimeFast = 50.0;
  bool slowOn = false;
  double fallTimeSlow = 100.0;
  double slowPct = 20.0;
  double recoveryTime = 50.0;

  // PDE
  PdeMode pdeMode = PdeMode::kNone;
  double pdePct = 30.0;
  std::string pdeFile;
  std::vector<double> pdeWavelengths;
  std::vector<double> pdeValues;

  // Source event timing: independent events (one per window, no pile-up) or
  // a stream of events at a given rate where each window is triggered by an
  // event and may contain more (pile-up)
  TimingMode timing = TimingMode::kIndependent;
  double rateKHz = 100.0;

  // Number of photons per event
  NPhotonMode nphMode = NPhotonMode::kPoisson;
  int nphFixed = 10;
  double nphMu = 10.0;
  std::string nphFile;
  std::vector<double> nphHistogram; // relative counts of bin i
  double nphScale = 1.0;            // photons per bin (photons/keV for energy spectra)

  // Gamma source seen by a scintillator crystal: the deposited energy is drawn
  // from a precomputed spectrum (source x material x size), the photon times
  // and wavelengths from the material
  std::string gammaFile;             // emission lines of the source
  ScintMaterial scint;               // selected material (fixed properties)
  double lightYieldScale = 0.05;     // multiplies the material light yield (collection, coupling)
  std::vector<double> gammaSpectrum; // deposited energy, 1 keV bins

  // Photon arrival times
  TimeMode timeMode = TimeMode::kFixed;
  double timeOffset = 100.0;
  double timeWidth = 20.0; // uniform
  double timeSigma = 2.0;  // gaussian
  double timeTau = 20.0;   // exponential / scintillator decay
  double timeRise = 1.0;   // scintillator rise

  // Photon wavelengths (only relevant for spectrum PDE)
  WavelengthMode wlMode = WavelengthMode::kGaussian;
  double wlFixed = 420.0;
  double wlMin = 350.0;
  double wlMax = 600.0;
  double wlMean = 420.0;
  double wlSigma = 30.0;

  // Analysis
  double intStart = 95.0;
  double intWindow = 250.0;
  double threshold = 0.5;
};

} // namespace sipmgui
#endif
