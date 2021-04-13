#include "SiPMDigitalSignal.h"

#include <algorithm>
#include <numeric>

namespace sipm {
/**
* Integral of the signal defined as the sum of all samples in the integration
* window normalized for the sampling time. If the signal is below the threshold
* the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Threshold to use for one-suppression
*/
int32_t SiPMDigitalSignal::integral(const double intstart, const double intgate, const int32_t threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return 0;
  }

  return std::accumulate(start, end, static_cast<int32_t>(0)) * m_Sampling;
}

/**
* Peak of the signal defined as sample with maximum amplitude in the integration
* gate.
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Threshold to use for one-suppression
*/
int32_t SiPMDigitalSignal::peak(const double intstart, const double intgate, const int32_t threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return 0;
  }
  return peak;
}

/**
* Time over threshold of the signal in the integration gate defined as the
* number of samples higher than the threshold normalized for the sampling time.
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Threshold to use for one-suppression
*/
double SiPMDigitalSignal::tot(const double intstart, const double intgate, const int32_t threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return 0;
  }

  double tot = 0;
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
@param threshold  Threshold to use for one-suppression
*/
double SiPMDigitalSignal::toa(const double intstart, const double intgate, const int32_t threshold) const {

  auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return -1;
  }

  double toa = 0;
  while (*start < threshold || start != end) {
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
@param threshold  Threshold to use for one-suppression
*/
double SiPMDigitalSignal::top(const double intstart, const double intgate, const int32_t threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return 0;
  }

  return (std::max_element(start, end) - start) * m_Sampling;
}
} // namespace sipm
