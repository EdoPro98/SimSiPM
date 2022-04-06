#include "SiPMAnalogSignal.h"
#include "SiPMMath.h"
#include "SiPMTypes.h"

namespace sipm {

template <> SiPMVector<float> SiPMAnalogSignal::waveform<SiPMVector<float>>() const { return m_Waveform; }

template <> std::vector<float> SiPMAnalogSignal::waveform<std::vector<float>>() const {
  std::vector<float> l_Waveform(m_Waveform.begin(), m_Waveform.end());
  return l_Waveform;
}

/**
* Integral of the signal defined as the sum of all samples in the integration
* window normalized for the sampling time. If the signal is below the threshold
* the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::integral(const double intstart, const double intgate, const double threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (this->peak(intstart, intgate, threshold) < threshold) {
    return -1;
  }
  const double integral = std::accumulate(start, end, 0.0) * m_Sampling;
  return integral;
}

/**
* Peak of the signal defined as sample with maximum amplitude in the integration
* gate.
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::peak(const double intstart, const double intgate, const double threshold) const {
  // Cache value for use in other functions without calculating again
  if (m_peak != 0) {
    return m_peak;
  }
  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    m_peak = -1;
    return -1;
  }
  m_peak = peak;
  return peak;
}

/**
* Time over threshold of the signal in the integration gate defined as the
* number of samples higher than the threshold normalized for the sampling time.
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::tot(const double intstart, const double intgate, const double threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (this->peak(intstart, intgate, threshold) < threshold) {
    return -1;
  }

  uint32_t tot = 0;
  for (auto itr = start; itr != end; ++itr) {
    // Add 1 if condition is true
    tot += (int)(*itr > threshold);
  }
  return tot * m_Sampling;
}

/**
* Arriving time of the signal defined as the time in ns of the first sample
* above the threshold.
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::toa(const double intstart, const double intgate, const double threshold) const {

  auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (this->peak(intstart, intgate, threshold) < threshold) {
    return -1;
  }

  uint32_t toa = 0;
  while (*start < threshold && start != end) {
    ++toa;
    ++start;
  }

  return toa * m_Sampling;
}

/**
* Time in ns of the sample in the peak
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::top(const double intstart, const double intgate, const double threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (this->peak(intstart, intgate, threshold) < threshold) {
    return -1;
  }

  return (std::max_element(start, end) - start) * m_Sampling;
}

std::ostream& operator<<(std::ostream& out, const SiPMAnalogSignal& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Analog Signal <===\n";
  out << "Address: " << std::addressof(obj) << "\n";
  out << "Signal length is: " << obj.m_Waveform.size() / obj.m_Sampling << " ns\n";
  out << "Signal is sampled every: " << obj.m_Sampling << " ns\n";
  out << "Signal contains: " << obj.m_Waveform.size() << " points";
  return out;
}
} // namespace sipm
