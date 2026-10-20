#ifndef SIPM_GUI_SOURCE_H
#define SIPM_GUI_SOURCE_H

#include <cstdint>
#include <deque>
#include <vector>

#include "Config.h"
#include "SiPMRandom.h"

namespace sipmgui {

/// Photons hitting the sensor in one acquisition window.
struct PhotonEvent {
  std::vector<double> times;       // ns, relative to the start of the simulated window
  std::vector<double> wavelengths; // empty unless spectrum PDE is active
  uint32_t nSources = 0;           // source events with light in the visible window (pile-up if > 1)
};

/// Photons emitted by one source event (count, arrival times, wavelengths).
class PhotonSource {
public:
  explicit PhotonSource(const SimConfig& c);
  /// Appends the photons of a source event happening at time t0 [ns]
  void emit(double t0, std::vector<double>& times, std::vector<double>& wavelengths, sipm::SiPMRandom& rng) const;

private:
  uint32_t nPhotons(sipm::SiPMRandom& rng) const;
  double time(sipm::SiPMRandom& rng) const;
  double wavelength(sipm::SiPMRandom& rng) const;

  const SimConfig& m_Cfg;
  bool m_Gamma;                   // photons from a scintillator crystal
  std::vector<double> m_Cdf;      // photon number histogram or deposited energy spectrum
  double m_LogQ = -1;             // geometric distribution
  std::vector<double> m_DecayCdf; // cumulative fractions of the material decay components
};

/// Turns the source into a sequence of acquisition windows of length
/// signalLength.
///
/// - Independent: one source event per window, no pile-up
/// - Poisson / Periodic: source events form a continuous timeline at the given
///   rate. A source event opens a window (time offset = pre-trigger) and every
///   other event whose light falls in it is included, so pulses can pile up.
///   The next window can only open after the current one (dead time = window).
///   Every window also covers a `margin` of history before its start, so the
///   sensor sees tails of earlier pulses, partially recharged cells and late
///   afterpulses; that part is simulated and then cropped away.
class EventStream {
public:
  /// `c` must outlive the stream
  explicit EventStream(const SimConfig& c);
  /// History simulated before each window [ns], multiple of the sampling time
  double margin() const { return m_Margin; }
  /// Produces the next window
  void next(PhotonEvent& out, sipm::SiPMRandom& rng);

private:
  struct Source {
    double t0;   // event time
    double tMax; // last photon time
    std::vector<double> times, wavelengths;
  };
  void generateOne(sipm::SiPMRandom& rng);
  void generateUntil(double t, sipm::SiPMRandom& rng);

  const SimConfig& m_Cfg;
  PhotonSource m_Photons;
  double m_Margin = 0;
  double m_Interval = 0;   // mean time between source events [ns]
  double m_NextSource = 0; // time of the next source event to generate
  double m_NextWindow = 0; // end of the dead time of the last window
  std::deque<Source> m_Pending;
};

} // namespace sipmgui
#endif
