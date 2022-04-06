#include "SiPMProperties.h"
#include <algorithm>
#include <cctype>

namespace sipm {

// Getters
uint32_t SiPMProperties::nCells() const {
  // m_SideCells and m_Ncells are cached
  if ((m_SideCells == 0) || (m_Ncells == 0)) {
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }
  return m_Ncells;
}

uint32_t SiPMProperties::nSideCells() const {
  // m_SideCells and m_Ncells are cached
  if ((m_SideCells == 0) || (m_Ncells == 0)) {
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }
  return m_SideCells;
}

uint32_t SiPMProperties::nSignalPoints() const {
  // m_Signalpoints is cached
  if (m_SignalPoints == 0) {
    m_SignalPoints = m_SignalLength / m_Sampling;
  }
  return m_SignalPoints;
}

double SiPMProperties::snrLinear() const {
  // m_SnrLinear is cached
  if (m_SnrLinear == 0) {
    m_SnrLinear = pow(10, -m_SnrdB / 20);
  }
  return m_SnrLinear;
}

// Setters
void SiPMProperties::setProperty(const std::string& prop, const double val) {
  // Make prop lowercase to avoid case-sensitive property mismatch
  std::string aProp(prop);
  std::transform(prop.begin(), prop.end(), aProp.begin(), [](char c) -> char { return std::tolower(c); });

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
    std::cerr << "Property: " << aProp << " not found!" << std::endl;
  }
}

void SiPMProperties::setSampling(const double val) {
  m_Sampling = val;
  m_SignalPoints = static_cast<uint32_t>(m_SignalLength / m_SignalPoints);
}

void SiPMProperties::setPdeSpectrum(const std::vector<double>& wav, const std::vector<double>& pde) {
  static constexpr uint32_t N = 25;

  std::map<double, double> interpolatedSpectrum;
  std::map<double, double> x;

  for (uint32_t i = 0, n = wav.size(); i < n; ++i) {
    x.emplace(wav[i], pde[i]);
  }

  interpolatedSpectrum = x;

  const double xmin = x.begin()->first;
  const double xmax = x.rbegin()->first;
  const double dx = (xmax - xmin) / 25;
  for (uint32_t i = 0; i < N; ++i) {
    const double newx = xmin + i * dx;
    if (interpolatedSpectrum.count(newx) > 0) { // Skip in case value exist
      continue;
    }
    auto it1 = x.upper_bound(newx);
    // Avoid boundary conditions
    if (it1 == x.end()) {
      --it1;
    }
    if (it1 == x.begin()) {
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
    if (newy == 0) { // Newy cannot be 0 becouse we are avoiding boundary conditions
      // Linear interpolation
      const double m = (y1 - y0) / (x1 - x0);
      const double q = y0 - m * x0;
      newy = m * newx + q;
    }
    interpolatedSpectrum.emplace(newx, newy);
  }

  m_PdeSpectrum = interpolatedSpectrum;
  m_HasPde = PdeType::kSpectrumPde;
}

void SiPMProperties::readSettings(std::string& fname) {
  std::ifstream cFile(fname);
  if (cFile.is_open()) {
    std::string line;
    while (getline(cFile, line)) {
      // Remove spaces
      line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
      // Ignore lines starting with # or /
      if (line[0] == '#' || line[0] == '/' || line.empty()) {
        continue;
      }
      // Get prop - value
      auto delimiterPos = line.find("=");
      auto varName = line.substr(0, delimiterPos);
      auto varValue = line.substr(delimiterPos + 1);

      setProperty(varName, std::stod(varValue));
    }
  } else {
    std::cerr << "Could not open " << fname << " for reading!" << std::endl;
  }
}

std::ostream& operator<<(std::ostream& out, const SiPMProperties& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Properties <===" << '\n';
  out << "Address: " << std::addressof(obj) << "\n";
  out << "Size: " << obj.m_Size << " mm\n";
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
