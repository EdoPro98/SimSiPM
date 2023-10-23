#include "SiPMAnalogSignal.h"
#include "SiPMTypes.h"

namespace sipm {

/**
* Integral of the signal defined as the sum of all samples in the integration
* window normalized for the sampling time. If the signal is below the threshold
* the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::integral(const double intstart, const double intgate, const double threshold) const {
  double integral = 0;
  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (std::any_of(start, end, [threshold](const double sample) { return sample > threshold; }) == false) {
    return -1;
  }
  for (auto itr = start; itr != end; ++itr) {
    integral += *itr;
  }
  return integral * m_Sampling;
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
  double peak = 0;
  const auto start = m_Waveform.cbegin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (std::any_of(start, end, [threshold](const double sample) { return sample > threshold; }) == false) {
    return -1;
  }
  for (auto itr = start; itr != end; ++itr) {
    if (*itr > peak) {
      peak = *itr;
    }
  }
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
  uint32_t tot = 0;
  const auto start = m_Waveform.cbegin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (std::any_of(start, end, [threshold](const double sample) { return sample > threshold; }) == false) {
    return -1;
  }
  for (auto itr = start; itr != end; ++itr) {
    if (*itr > threshold) {
      ++tot;
    }
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
  uint32_t toa = 0;
  auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (std::any_of(start, end, [threshold](const double sample) { return sample > threshold; }) == false) {
    return -1;
  }

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
  const auto start = m_Waveform.cbegin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  if (std::any_of(start, end, [threshold](const double sample) { return sample > threshold; }) == false) {
    return -1;
  }

  return static_cast<double>(std::max_element(start, end) - start) * m_Sampling;
}

std::ostream& operator<<(std::ostream& out, const SiPMAnalogSignal& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Analog Signal <===\n";
  out << "Address: " << std::hex << std::addressof(obj) << "\n";
  out << "Signal length is: " << std::dec << obj.m_Waveform.size() / obj.m_Sampling << " ns\n";
  out << "Signal is sampled every: " << obj.m_Sampling << " ns\n";
  out << "Signal contains: " << obj.m_Waveform.size() << " points";
  return out;
}
} // namespace sipm
