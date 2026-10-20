#include "Source.h"

#include <algorithm>
#include <cmath>

namespace sipmgui {
// ============================================================= PhotonSource

PhotonSource::PhotonSource(const SimConfig& c) : m_Cfg(c), m_Gamma(c.nphMode == NPhotonMode::kGamma) {
  if (c.nphMode == NPhotonMode::kFile || m_Gamma) {
    double sum = 0;
    for (const double v : m_Gamma ? c.gammaSpectrum : c.nphHistogram) {
      sum += v;
      m_Cdf.push_back(sum);
    }
  }
  if (m_Gamma) {
    double sum = 0;
    for (const auto& [tau, frac] : c.scint.decay) {
      sum += frac;
      m_DecayCdf.push_back(sum);
    }
  }
  if (c.nphMode == NPhotonMode::kExponential && c.nphMu > 0) {
    // Geometric distribution with mean mu: discrete analogue of the exponential
    m_LogQ = std::log(c.nphMu / (1.0 + c.nphMu));
  }
}

void PhotonSource::emit(double t0, std::vector<double>& times, std::vector<double>& wavelengths,
                        sipm::SiPMRandom& rng) const {
  const uint32_t n = nPhotons(rng);
  const double base = t0 + m_Cfg.timeOffset;
  for (uint32_t i = 0; i < n; ++i) {
    times.push_back(base + time(rng));
  }
  // Wavelengths are only needed when the PDE depends on them
  if (m_Cfg.pdeMode == PdeMode::kSpectrum) {
    for (uint32_t i = 0; i < n; ++i) {
      wavelengths.push_back(wavelength(rng));
    }
  }
}

uint32_t PhotonSource::nPhotons(sipm::SiPMRandom& rng) const {
  switch (m_Cfg.nphMode) {
    case NPhotonMode::kFixed:
      return static_cast<uint32_t>(std::max(0, m_Cfg.nphFixed));
    case NPhotonMode::kPoisson:
      return rng.randPoisson(m_Cfg.nphMu);
    case NPhotonMode::kExponential:
      if (m_Cfg.nphMu <= 0) {
        return 0;
      }
      return static_cast<uint32_t>(std::floor(std::log(rng.Rand()) / m_LogQ));
    case NPhotonMode::kFile:
      {
        if (m_Cdf.empty() || m_Cdf.back() <= 0) {
          return 0;
        }
        // u in (0, total]: first bin whose cumulative count reaches u (never an empty bin)
        const double u = rng.Rand() * m_Cdf.back();
        const auto bin = static_cast<uint32_t>(std::lower_bound(m_Cdf.begin(), m_Cdf.end(), u) - m_Cdf.begin());
        if (m_Cfg.nphScale == 1.0) {
          return bin;
        }
        // Stochastic rounding keeps the mean exact; this needs a uniform in [0,1)
        return static_cast<uint32_t>(std::floor(bin * m_Cfg.nphScale + (1.0 - rng.Rand())));
      }
    case NPhotonMode::kGamma:
      {
        if (m_Cdf.empty() || m_Cdf.back() <= 0) {
          return 0;
        }
        // Deposited energy from the precomputed spectrum (uniform inside the
        // 1 keV bin, which needs [0,1)); scintillation photons are Poisson
        const double u = rng.Rand() * m_Cdf.back();
        const auto bin = std::lower_bound(m_Cdf.begin(), m_Cdf.end(), u) - m_Cdf.begin();
        const double energy = bin + (1.0 - rng.Rand());
        return rng.randPoisson(energy * m_Cfg.scint.lightYield * m_Cfg.lightYieldScale);
      }
  }
  return 0;
}

double PhotonSource::time(sipm::SiPMRandom& rng) const {
  if (m_Gamma) {
    // Material scintillation: rise plus one of the decay components
    double t = m_Cfg.scint.riseTime > 0 ? rng.randExponential(m_Cfg.scint.riseTime) : 0;
    if (!m_DecayCdf.empty()) {
      const double u = rng.Rand() * m_DecayCdf.back();
      const size_t k = std::lower_bound(m_DecayCdf.begin(), m_DecayCdf.end(), u) - m_DecayCdf.begin();
      t += rng.randExponential(m_Cfg.scint.decay[k].first);
    }
    return t;
  }
  switch (m_Cfg.timeMode) {
    case TimeMode::kFixed:
      return 0;
    case TimeMode::kUniform:
      return rng.Rand() * m_Cfg.timeWidth;
    case TimeMode::kGaussian:
      return rng.randGaussian(0, m_Cfg.timeSigma);
    case TimeMode::kExponential:
      return rng.randExponential(m_Cfg.timeTau);
    case TimeMode::kScintillator:
      // Sum of two exponentials has a (exp(-t/decay) - exp(-t/rise)) pdf
      return rng.randExponential(m_Cfg.timeTau) + rng.randExponential(m_Cfg.timeRise);
  }
  return 0;
}

double PhotonSource::wavelength(sipm::SiPMRandom& rng) const {
  if (m_Gamma) {
    return rng.randGaussian(m_Cfg.scint.emissionPeak, m_Cfg.scint.emissionFwhm / 2.3548);
  }
  switch (m_Cfg.wlMode) {
    case WavelengthMode::kFixed:
      return m_Cfg.wlFixed;
    case WavelengthMode::kUniform:
      return m_Cfg.wlMin + rng.Rand() * (m_Cfg.wlMax - m_Cfg.wlMin);
    case WavelengthMode::kGaussian:
      return rng.randGaussian(m_Cfg.wlMean, m_Cfg.wlSigma);
  }
  return m_Cfg.wlFixed;
}

// ============================================================== EventStream

EventStream::EventStream(const SimConfig& c) : m_Cfg(c), m_Photons(c) {
  if (c.timing == TimingMode::kIndependent) {
    return;
  }
  // History long enough for pulse tails and cell recovery to fade out
  double tau = std::max(c.fallTimeFast, c.recoveryTime);
  if (c.slowOn) {
    tau = std::max(tau, c.fallTimeSlow);
  }
  m_Margin = std::ceil(std::min(5 * tau, 5000.0) / c.sampling) * c.sampling;
  m_Interval = 1e6 / std::max(c.rateKHz, 1e-6); // kHz -> ns
  m_NextSource = 0;
}

void EventStream::generateOne(sipm::SiPMRandom& rng) {
  Source s;
  s.t0 = m_NextSource;
  m_Photons.emit(s.t0, s.times, s.wavelengths, rng);
  s.tMax = s.times.empty() ? s.t0 : *std::max_element(s.times.begin(), s.times.end());
  m_Pending.push_back(std::move(s));
  m_NextSource += m_Cfg.timing == TimingMode::kPoisson ? rng.randExponential(m_Interval) : m_Interval;
}

void EventStream::generateUntil(double t, sipm::SiPMRandom& rng) {
  while (m_NextSource < t) {
    generateOne(rng);
  }
}

void EventStream::next(PhotonEvent& out, sipm::SiPMRandom& rng) {
  out.times.clear();
  out.wavelengths.clear();
  if (m_Cfg.timing == TimingMode::kIndependent) {
    m_Photons.emit(0, out.times, out.wavelengths, rng);
    out.nSources = 1;
    return;
  }

  const double length = m_Cfg.signalLength;
  // Trigger: the first source event after the dead time opens the window
  auto first = [&] {
    return std::find_if(m_Pending.begin(), m_Pending.end(), [&](const Source& s) { return s.t0 >= m_NextWindow; });
  };
  while (first() == m_Pending.end()) {
    generateOne(rng);
  }
  const double start = first()->t0;
  m_NextWindow = start + length;
  generateUntil(start + length, rng);

  const double base = start - m_Margin;
  while (!m_Pending.empty() && m_Pending.front().tMax < base) {
    m_Pending.pop_front();
  }
  const bool withWavelengths = m_Cfg.pdeMode == PdeMode::kSpectrum;
  out.nSources = 0;
  for (const Source& s : m_Pending) {
    bool visible = false;
    for (size_t i = 0; i < s.times.size(); ++i) {
      const double t = s.times[i];
      if (t >= base && t < start + length) {
        visible |= t >= start;
        out.times.push_back(t - base);
        if (withWavelengths) {
          out.wavelengths.push_back(s.wavelengths[i]);
        }
      }
    }
    out.nSources += visible;
  }
}

} // namespace sipmgui
