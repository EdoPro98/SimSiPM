#include "SiPMProperties.h"

#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <math.h>

namespace sipm {

// Getters
uint32_t SiPMProperties::nCells() const {
  if ((m_SideCells == 0) || (m_Ncells == 0)) {
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }
  return m_Ncells;
}

uint32_t SiPMProperties::nSideCells() const {
  if ((m_SideCells == 0) || (m_Ncells == 0)) {
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }
  return m_SideCells;
}

uint32_t SiPMProperties::nSignalPoints() const {
  if (m_SignalPoints == 0) {
    m_SignalPoints = m_SignalLength / m_Sampling;
  }
  return m_SignalPoints;
}

double SiPMProperties::snrLinear() const {
  if (m_SnrLinear == 0) {
    m_SnrLinear = pow(10, -m_SnrdB / 20);
  }
  return m_SnrLinear;
}

// Setters
void SiPMProperties::setProperty(const std::string &aProp, const double aPropValue) {
  if (aProp == "Size") {
    setSize(aPropValue);
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  } else if (aProp == "Pitch") {
    setPitch(aPropValue);
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  } else if (aProp == "Sampling") {
    setSampling(aPropValue);
    m_SignalPoints = m_SignalLength / m_Sampling;
  } else if (aProp == "CellRecovery") {
    setRecoveryTime(aPropValue);
  } else if (aProp == "SignalLength") {
    setSignalLength(aPropValue);
    m_SignalPoints = m_SignalLength / m_Sampling;
  } else if (aProp == "RiseTime") {
    setRiseTime(aPropValue);
  } else if (aProp == "FallTimeFast") {
    setFallTimeFast(aPropValue);
  } else if (aProp == "FallTimeSlow") {
    setFallTimeSlow(aPropValue);
  } else if (aProp == "SlowComponentFraction") {
    setSlowComponentFraction(aPropValue);
  } else if (aProp == "RecoveryTime") {
    setRecoveryTime(aPropValue);
  } else if (aProp == "TauApFast") {
    setTauApFastComponent(aPropValue);
  } else if (aProp == "TauApSlow") {
    setTauApSlowComponent(aPropValue);
  } else if (aProp == "Ccgv") {
    setCcgv(aPropValue);
  } else if (aProp == "Snr") {
    setSnr(aPropValue);
    m_SnrLinear = pow(10, -m_SnrdB / 20);
  } else if (aProp == "Pde") {
    setPde(aPropValue);
  } else if (aProp == "Dcr") {
    setDcr(aPropValue);
  } else if (aProp == "Xt") {
    setXt(aPropValue);
  } else if (aProp == "Ap") {
    setAp(aPropValue);
  } else {
    std::cerr << "Property: " << aProp << " not found! \n";
  }
}

void SiPMProperties::setSampling(const double aSampling) {
  m_Sampling = aSampling;
  m_SignalPoints = static_cast<uint32_t>(m_SignalLength / aSampling);
}

void SiPMProperties::setPdeSpectrum(const std::map<double, double> &x) {
  m_PdeSpectrum = x;
  m_HasPde = PdeType::kSpectrumPde;
}

void SiPMProperties::setPdeSpectrum(const std::vector<double> &wav, const std::vector<double> &pde) {
  for (uint32_t i = 0; i < wav.size(); ++i) {
    m_PdeSpectrum[wav[i]] = pde[i];
  }
  m_HasPde = PdeType::kSpectrumPde;
}

void SiPMProperties::readSettings(std::string &fname) {
  std::ifstream cFile(fname);
  if (cFile.is_open()) {
    std::string line;
    while (getline(cFile, line)) {
      line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
      if (line[0] == '#' || line.empty()) {
        continue;
      }
      auto delimiterPos = line.find("=");
      auto varName = line.substr(0, delimiterPos);
      auto varValue = line.substr(delimiterPos + 1);

      setProperty(varName, std::stod(varValue));
    }
  } else {
    std::cerr << "Could not open config file for reading.\n";
  }
}

void SiPMProperties::dumpSettings() const {
  std::cout << "===> SiPM Settings <===" << '\n';
  std::cout << "Size: " << m_Size << " mm\n";
  std::cout << "Pitch: " << m_Pitch << " um\n";
  std::cout << "Number of cells: " << nCells() << "\n";
  switch (m_HitDistribution) {
    case (HitDistribution::kUniform):
      std::cout << "Hit distribution: "
                << "Uniform"
                << "\n";
      break;
    case (HitDistribution::kCircle):
      std::cout << "Hit distribution: "
                << "Circle"
                << "\n";
      break;
    case (HitDistribution::kGaussian):
      std::cout << "Hit distribution: "
                << "Gaussian"
                << "\n";
      break;
  }
  std::cout << "Cell recovery time: " << m_RecoveryTime << " nm\n";
  if (m_HasDcr) {
    std::cout << "Dark count rate: " << m_Dcr / 1e3 << " kHz\n";
  } else {
    std::cout << "Dark count rate: Off\n";
  }
  if (m_HasXt) {
    std::cout << "Crosstalk probability: " << m_Xt * 100 << " %\n";
  } else {
    std::cout << "Crosstalk probability: Off\n";
  }
  if (m_HasAp) {
    std::cout << "Afterpulse probability: " << m_Ap * 100 << " %\n";
    std::cout << "Tau afterpulses (fast): " << m_TauApFastComponent << " ns\n";
    std::cout << "Tau afterpulses (slow): " << m_TauApSlowComponent << " ns\n";
  } else {
    std::cout << "Afterpulse probability: Off\n";
  }
  std::cout << "Cell-to-cell gain variation: " << m_Ccgv * 100 << " %\n";
  std::cout << "SNR: " << m_SnrdB << " dB\n";
  if (m_HasPde == PdeType::kSimplePde) {
    std::cout << "Photon detection efficiency: " << m_Pde * 100 << " %\n";
  } else if (m_HasPde == PdeType::kSimplePde) {
    std::cout << "Photon detection efficiency: depending on wavelength\n";
  } else {
    std::cout << "Photon detection efficiency: Off\n";
  }
  std::cout << "Rising time of signal: " << m_RiseTime << " ns\n";
  std::cout << "Falling time of signal (fast): " << m_FallTimeFast << " ns\n";
  if (m_HasSlowComponent) {
    std::cout << "Falling time of signal (slow): " << m_FallTimeSlow << " ns\n";
    std::cout << "Slow component fraction: " << m_SlowComponentFraction * 100 << " %\n";
  }
  std::cout << "Signal length: " << m_SignalLength << " ns\n";
  std::cout << "Sampling time: " << m_Sampling << " ns\n";
  std::cout << "==> End of SiPM Settings <===" << '\n';
}
} // namespace sipm
