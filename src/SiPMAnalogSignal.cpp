#include "SiPMAnalogSignal.h"

#include <algorithm>
#include <math.h>
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
double SiPMAnalogSignal::integral(const double intstart, const double intgate, const double threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return 0;
  }
  return std::accumulate(start, end, 0.0) * m_Sampling;
}

/**
* Peak of the signal defined as sample with maximum amplitude in the integration
* gate.
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Threshold to use for one-suppression
*/
double SiPMAnalogSignal::peak(const double intstart, const double intgate, const double threshold) const {

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
double SiPMAnalogSignal::tot(const double intstart, const double intgate, const double threshold) const {

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
double SiPMAnalogSignal::toa(const double intstart, const double intgate, const double threshold) const {

  auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return -1;
  }

  double toa = 0;
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
@param threshold  Threshold to use for one-suppression
*/
double SiPMAnalogSignal::top(const double intstart, const double intgate, const double threshold) const {

  const auto start = m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  const double peak = *std::max_element(start, end);
  if (peak < threshold) {
    return 0;
  }

  return (std::max_element(start, end) - start) * m_Sampling;
}

/**
 * @param bw Bandwidth for the low-pass filter (-3dB cut-off)
 * @return Signal with filter applied
 */
SiPMAnalogSignal SiPMAnalogSignal::lowpass(const double bw) const {
  std::vector<double> out = m_Waveform;
  const double rc = 1 / (2 * M_PI * bw);
  const double dt = 1e-9 * m_Sampling;
  const double alpha = dt / (rc + dt);

  // Implementation of a first-order low-pass filter
  out[0] = alpha * out[0];
  for (uint32_t i = 1; i < out.size(); ++i) {
    out[i] = alpha * (out[i] - out[i - 1]) + out[i - 1];
  }
  return SiPMAnalogSignal(out, m_Sampling);
}
} // namespace sipm
