#include "SiPMProperties.h"
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sipm {

SiPMProperties::SiPMProperties() {
  m_SideCells = 1000 * m_Size / m_Pitch;
  m_Ncells = m_SideCells * m_SideCells;
  m_SignalPoints = m_SignalLength / m_Sampling;
  m_SnrLinear = pow(10, -m_SnrdB / 20);
}

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

void SiPMProperties::setPdeSpectrum(const std::vector<double>& wav, const std::vector<double>& pde) {
  static constexpr uint32_t N = 32;
  std::map<double, double> inputSpectrum;

  const size_t n = std::min(wav.size(), pde.size());
  if (wav.size() != pde.size()) {
    std::cerr << "setPdeSpectrum: wavelengths and pde have different sizes (" << wav.size() << " vs " << pde.size()
               << "), extra entries are ignored" << std::endl;
  }
  for (size_t i = 0; i < n; ++i) {
    inputSpectrum[wav[i]] = pde[i];
  }

  const double xmin = inputSpectrum.cbegin()->first;
  const double xmax = inputSpectrum.crbegin()->first;
  const double dx = (xmax - xmin) / N;
  for (uint32_t i = 0; i < N; ++i) {
    const double newx = xmin + i * dx;
    auto it1 = inputSpectrum.upper_bound(newx);
    // Avoid boundary conditions
    if (it1 == inputSpectrum.cend()) {
      --it1;
    }
    if (it1 == inputSpectrum.cbegin()) {
      ++it1;
    }
    auto it0 = it1;
    --it0;
    const double x0 = it0->first;
    const double x1 = it1->first;
    const double y0 = it0->second;
    const double y1 = it1->second;
    // Interpolation is done in log space: if one of the two input points has
    // zero PDE the interpolated value is zero (avoids log(0) = -inf)
    if (y0 <= 0 || y1 <= 0) {
      m_PdeSpectrum[newx] = (newx == x0 && y0 > 0) ? y0 : 0;
      continue;
    }
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
    m_PdeSpectrum[newx] = newy;
  }

  m_HasPde = PdeType::kSpectrumPde;
}

SiPMProperties SiPMProperties::readSettings(const std::string& fname) {
  SiPMProperties retval;
  std::ifstream file(fname);
  if (file.is_open()) {
    std::string line;
    while (getline(file, line)) {
      if (line.empty()) {
        continue;
      }
      // Remove spaces
      line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
      // Ignore lines starting with # or /
      if (line[0] == '#' || line[0] == '/') {
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

SiPMProperties SiPMProperties::readSettingsJson(const std::string& fname) {
  SiPMProperties p;
  std::ifstream file(fname);
  if (!file.is_open()) {
    std::cerr << "Could not open " << fname << " for reading!\n";
    return p;
  }

  nlohmann::json j;
  try {
    file >> j;
  } catch (const nlohmann::json::parse_error& e) {
    std::cerr << "JSON parse error in " << fname << ": " << e.what() << "\n";
    return p;
  }

  if (j.contains("size"))                  p.setSize(j["size"].get<double>());
  if (j.contains("pitch"))                 p.setPitch(j["pitch"].get<double>());
  if (j.contains("sampling"))              p.setSampling(j["sampling"].get<double>());
  if (j.contains("signalLength"))          p.setSignalLength(j["signalLength"].get<double>());
  if (j.contains("risingTime"))            p.setRiseTime(j["risingTime"].get<double>());
  if (j.contains("fallingTimeFast"))       p.setFallTimeFast(j["fallingTimeFast"].get<double>());
  if (j.contains("fallingTimeSlow"))       p.setFallTimeSlow(j["fallingTimeSlow"].get<double>());
  if (j.contains("slowComponentFraction")) p.setSlowComponentFraction(j["slowComponentFraction"].get<double>());
  if (j.contains("recoveryTime"))          p.setRecoveryTime(j["recoveryTime"].get<double>());
  if (j.contains("snr"))                   p.setSnr(j["snr"].get<double>());
  if (j.contains("dcr"))                   p.setDcr(j["dcr"].get<double>());
  if (j.contains("xt"))                    p.setXt(j["xt"].get<double>());
  if (j.contains("dxt"))                   p.setDXt(j["dxt"].get<double>());
  if (j.contains("dxtTau"))                p.setDXtTau(j["dxtTau"].get<double>());
  if (j.contains("ap"))                    p.setAp(j["ap"].get<double>());
  if (j.contains("tauApFast"))             p.setTauApFastComponent(j["tauApFast"].get<double>());
  if (j.contains("tauApSlow"))             p.setTauApSlowComponent(j["tauApSlow"].get<double>());
  if (j.contains("apSlowFraction"))        p.setApSlowFraction(j["apSlowFraction"].get<double>());
  if (j.contains("ccgv"))                  p.setCcgv(j["ccgv"].get<float>());
  if (j.contains("pde"))                   p.setPde(j["pde"].get<double>());

  if (j.contains("hasDcr"))          { j["hasDcr"].get<bool>()          ? p.setDcrOn()           : p.setDcrOff(); }
  if (j.contains("hasXt"))           { j["hasXt"].get<bool>()           ? p.setXtOn()            : p.setXtOff(); }
  if (j.contains("hasDXt"))          { j["hasDXt"].get<bool>()          ? p.setDXtOn()           : p.setDXtOff(); }
  if (j.contains("hasAp"))           { j["hasAp"].get<bool>()           ? p.setApOn()            : p.setApOff(); }
  if (j.contains("hasSlowComponent")) { j["hasSlowComponent"].get<bool>() ? p.setSlowComponentOn() : p.setSlowComponentOff(); }

  if (j.contains("pdeType")) {
    const auto& t = j["pdeType"];
    if (t.is_string()) {
      const std::string s = t.get<std::string>();
      if (s == "kSimplePde")       p.setPdeType(PdeType::kSimplePde);
      else if (s == "kSpectrumPde") p.setPdeType(PdeType::kSpectrumPde);
      else                          p.setPdeType(PdeType::kNoPde);
    } else {
      p.setPdeType(static_cast<PdeType>(t.get<int>()));
    }
  }

  if (j.contains("hitDistribution")) {
    const auto& h = j["hitDistribution"];
    if (h.is_string()) {
      const std::string s = h.get<std::string>();
      if (s == "kCircle")        p.setHitDistribution(HitDistribution::kCircle);
      else if (s == "kGaussian") p.setHitDistribution(HitDistribution::kGaussian);
      else                       p.setHitDistribution(HitDistribution::kUniform);
    } else {
      p.setHitDistribution(static_cast<HitDistribution>(h.get<int>()));
    }
  }

  if (j.contains("pdeSpectrum")) {
    const auto& spec = j["pdeSpectrum"];
    if (spec.contains("wavelengths") && spec.contains("pde")) {
      p.setPdeSpectrum(
        spec["wavelengths"].get<std::vector<double>>(),
        spec["pde"].get<std::vector<double>>());
    }
  }

  return p;
}

std::ostream& operator<<(std::ostream& out, const SiPMProperties& obj) {
  // The caller's stream format is restored before returning
  const std::ios_base::fmtflags flags = out.flags();
  const std::streamsize precision = out.precision();
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
  } else if (obj.m_HasPde == SiPMProperties::PdeType::kSpectrumPde) {
    out << "Photon detection efficiency: depending on wavelength\n";
    out << "Photon wavelength\tDetection efficiency\n";
    for (auto it = obj.m_PdeSpectrum.begin(); it != obj.m_PdeSpectrum.end(); ++it) {
      out << it->first << " -> " << it->second << "\n";
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
  out.flags(flags);
  out.precision(precision);
  return out;
}

} // namespace sipm
