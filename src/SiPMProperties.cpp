#include "SiPMProperties.h"

namespace sipm {
void SiPMProperties::setProperty(const std::string& prop, const double val) {
  // Make prop lowercase to avoid case-sensitive property mismatch
  std::string aProp(prop);
  std::transform(prop.cbegin(), prop.cend(), aProp.begin(), [](const char c) -> char { return std::tolower(c); });

  if (aProp == "size") {
    setSize(val);
  } else if (aProp == "pitch") {
    setPitch(val);
  } else if (aProp == "sampling") {
    setSampling(val);
  } else if (aProp == "cellrecovery") {
    setRecoveryTime(val);
  } else if (aProp == "signallength") {
    setSignalLength(val);
  } else if (aProp == "risetime") {
    setRiseTime(val);
  } else if (aProp == "falltimefast") {
    setFallTimeFast(val);
  } else if (aProp == "falltimeslow") {
    setFallTimeSlow(val);
  } else if (aProp == "slowcomponentfraction") {
    setSlowComponentFraction(val);
  } else if (aProp == "recoverytime") {
    setRecoveryTime(val);
  } else if (aProp == "tauapfast") {
    setTauApFastComponent(val);
  } else if (aProp == "tauapslow") {
    setTauApSlowComponent(val);
  } else if (aProp == "ccgv") {
    setCcgv(val);
  } else if (aProp == "snr") {
    setSnr(val);
  } else if (aProp == "pde") {
    setPde(val);
  } else if (aProp == "dcr") {
    setDcr(val);
  } else if (aProp == "xt") {
    setXt(val);
  } else if (aProp == "dxt") {
    setDXt(val);
  } else if (aProp == "ap") {
    setAp(val);
  } else {
    std::cerr << "Property: " << prop << " not found!" << std::endl;
  }
}

/// @param x Sampling time in ns
void SiPMProperties::setSampling(const double x) {
  m_Sampling = x;
  m_SignalPoints = static_cast<uint32_t>(m_SignalLength / m_Sampling);
}

void SiPMProperties::setPdeSpectrum(const std::vector<double>& wav, const std::vector<double>& pde) {
  static constexpr uint32_t N = 25;
  const uint32_t n = wav.size();
  std::map<double, double> interpolatedSpectrum;
  std::map<double, double> x;

  for (uint32_t i = 0; i < n; ++i) {
    x.emplace(wav[i], pde[i]);
  }

  interpolatedSpectrum = x;

  const double xmin = x.cbegin()->first;
  const double xmax = x.crbegin()->first;
  const double dx = (xmax - xmin) / 25;
  for (uint32_t i = 0; i < N; ++i) {
    const double newx = xmin + i * dx;
    auto it1 = x.upper_bound(newx);
    // Avoid boundary conditions
    if (it1 == x.cend()) {
      --it1;
    }
    if (it1 == x.cbegin()) {
      ++it1;
    }
    auto it0 = it1;
    --it0;
    const double x0 = it0->first;
    const double x1 = it1->first;
    const double y0 = it0->second;
    const double y1 = it1->second;
    const double logNewy = (log(y0) * log(x1 / newx) + log(y1) * log(newx / x0)) / log(x1 / x0);
    double newy = std::exp(logNewy);
    // If newy is 0 or less try linear interpolation
    if (newy < 0) {
      // Linear interpolation
      const double m = (y1 - y0) / (x1 - x0);
      const double q = y0 - m * x0;
      newy = m * newx + q;
    }
    // If newy < 0 just set at 0
    newy = newy < 0 ? 0 : newy;
    interpolatedSpectrum.emplace(newx, newy);
  }

  m_PdeSpectrum = interpolatedSpectrum;
  m_HasPde = PdeType::kSpectrumPde;
}

SiPMProperties SiPMProperties::readSettings(const std::string& fname) {
  SiPMProperties retval;
  std::ifstream file(fname);
  if (file.is_open()) {
    std::string line;
    while (getline(file, line)) {
      // Remove spaces
      line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
      // Ignore lines starting with # or /
      if (line[0] == '#' || line[0] == '/' || line.empty()) {
        continue;
      }
      // Get prop - value
      const auto delimiterPos = line.find('=');
      const auto varName = line.substr(0, delimiterPos);
      const auto varValue = line.substr(delimiterPos + 1);

      retval.setProperty(varName, std::stod(varValue));
    }
  } else {
    std::cerr << "Could not open " << fname << " for reading!" << std::endl;
  }
  return retval;
}

std::ostream& operator<<(std::ostream& out, const SiPMProperties& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Properties <===" << '\n';
  out << "Address: " << std::hex << std::addressof(obj) << "\n";
  out << "Size: " << std::dec << obj.m_Size << " mm\n";
  out << "Pitch: " << obj.m_Pitch << " um\n";
  out << "Number of cells: " << obj.nCells() << "\n";
  out << "Hit distribution: ";
  switch (obj.m_HitDistribution) {
    case (SiPMProperties::HitDistribution::kUniform):
      out << "Uniform\n";
      break;
    case (SiPMProperties::HitDistribution::kCircle):
      out << "Circle\n";
      break;
    case (SiPMProperties::HitDistribution::kGaussian):
      out << "Gaussian\n";
      break;
  }
  out << "Cell recovery time: " << obj.m_RecoveryTime << " ns\n";
  if (obj.m_HasDcr) {
    out << "Dark count rate: " << obj.m_Dcr / 1e3 << " kHz\n";
  } else {
    out << "Dark count is OFF\n";
  }
  if (obj.m_HasXt) {
    out << "Optical crosstalk probability: " << obj.m_Xt * 100 << " %\n";
  } else {
    out << "Optical crosstalk is OFF\n";
  }
  if (obj.m_HasDXt && obj.m_HasXt) {
    out << "Delayed optical crosstalk probability (as a fraction of xt): " << obj.m_DXt * 100 << " %\n";
  } else {
    out << "Delayed optical crosstalk is OFF\n";
  }
  if (obj.m_HasAp) {
    out << "Afterpulse probability: " << obj.m_Ap * 100 << " %\n";
    out << "Tau afterpulses (fast): " << obj.m_TauApFastComponent << " ns\n";
    out << "Tau afterpulses (slow): " << obj.m_TauApSlowComponent << " ns\n";
  } else {
    out << "Afterpulse is OFF\n";
  }
  out << "Cell-to-cell gain variation: " << obj.m_Ccgv * 100 << " %\n";
  out << "SNR: " << obj.m_SnrdB << " dB\n";
  if (obj.m_HasPde == SiPMProperties::PdeType::kSimplePde) {
    out << "Photon detection efficiency: " << obj.m_Pde * 100 << " %\n";
  } else if (obj.m_HasPde == SiPMProperties::PdeType::kSimplePde) {
    out << "Photon detection efficiency: depending on wavelength\n";
    out << "Photon wavelength\tDetection efficiency\n";
    for (auto it = obj.m_PdeSpectrum.begin(); it != obj.m_PdeSpectrum.end(); ++it) {
      out << it->first << "\t\t->\t\t" << it->second << "\n";
    }
  } else {
    out << "Photon detection efficiency is OFF (100 %)\n";
  }
  out << "Rising time of signal: " << obj.m_RiseTime << " ns\n";
  out << "Falling time of signal (fast): " << obj.m_FallTimeFast << " ns\n";
  if (obj.m_HasSlowComponent) {
    out << "Falling time of signal (slow): " << obj.m_FallTimeSlow << " ns\n";
    out << "Slow component fraction: " << obj.m_SlowComponentFraction * 100 << " %\n";
  }
  out << "Signal length: " << obj.m_SignalLength << " ns\n";
  out << "Sampling time: " << obj.m_Sampling << " ns\n";
  return out;
}

} // namespace sipm
