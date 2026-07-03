#include "SiPMAnalogSignal.h"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <numeric>

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
  const uint32_t startIdx = static_cast<uint32_t>(intstart / m_Sampling);
  if (startIdx >= m_Waveform.size()) return -1;
  const uint32_t endIdx = std::min(static_cast<uint32_t>((intstart + intgate) / m_Sampling),
                                   static_cast<uint32_t>(m_Waveform.size()));
  auto start = m_Waveform.begin() + startIdx;
  const auto end = m_Waveform.cbegin() + endIdx;
  bool isOver = false;
  float integral = 0;
  while (start < end) {
    const float v = *start++;
    if (v > threshold) { isOver = true; }
    integral += v;
  }
  return isOver ? integral * m_Sampling : -1;
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
  const uint32_t startIdx = static_cast<uint32_t>(intstart / m_Sampling);
  if (startIdx >= m_Waveform.size()) return -1;
  const uint32_t endIdx = std::min(static_cast<uint32_t>((intstart + intgate) / m_Sampling),
                                   static_cast<uint32_t>(m_Waveform.size()));
  auto start = m_Waveform.begin() + startIdx;
  const auto end = m_Waveform.cbegin() + endIdx;
  float peak = threshold;
  while (start < end) {
    const float v = *start++;
    if (v > peak) { peak = v; }
  }
  return peak > threshold ? peak : -1.0f;
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
  const uint32_t start = static_cast<uint32_t>(intstart / m_Sampling);
  if (start >= m_Waveform.size()) return -1;
  const uint32_t end = std::min(static_cast<uint32_t>((intstart + intgate) / m_Sampling),
                                static_cast<uint32_t>(m_Waveform.size()));

  double tot = 0;
  bool above = m_Waveform[start] > threshold;
  double riseTime = 0;

  for (uint32_t i = start + 1; i < end; ++i) {
    const bool currentAbove = m_Waveform[i] > threshold;
    if (!above && currentAbove) {
      const double d = (threshold - m_Waveform[i - 1]) / (m_Waveform[i] - m_Waveform[i - 1]);
      riseTime = (i - 1 - start + d) * m_Sampling;
      above = true;
    } else if (above && !currentAbove) {
      const double d = (threshold - m_Waveform[i - 1]) / (m_Waveform[i] - m_Waveform[i - 1]);
      tot += (i - 1 - start + d) * m_Sampling - riseTime;
      above = false;
    }
  }

  if (above) {
    tot += intgate - riseTime;
  }

  return tot > 0 ? tot : -1;
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
  const uint32_t start = static_cast<uint32_t>(intstart / m_Sampling);
  if (start >= m_Waveform.size()) return -1;
  const uint32_t end = std::min(static_cast<uint32_t>((intstart + intgate) / m_Sampling),
                                static_cast<uint32_t>(m_Waveform.size()));

  if (m_Waveform[start] > threshold) { return 0; }

  for (uint32_t i = start; i < end; ++i) {
    if (m_Waveform[i] > threshold) {
      const float d = (threshold - m_Waveform[i - 1]) / (m_Waveform[i] - m_Waveform[i - 1]);
      return (i - start - 1 + d) * m_Sampling;
    }
  }

  return -1;
}

/**
* Time in ns of the sample in the peak
* If the signal is below the threshold the output is set to -1.
@param intstart   Starting time of integration in ns
@param intgate    Length of the integration gate
@param threshold  Process only if above the threshold
*/
double SiPMAnalogSignal::top(const double intstart, const double intgate, const double threshold) const {
  const uint32_t start = static_cast<uint32_t>(intstart / m_Sampling);
  if (start >= m_Waveform.size()) return -1;
  const uint32_t end = std::min(static_cast<uint32_t>((intstart + intgate) / m_Sampling),
                                static_cast<uint32_t>(m_Waveform.size()));
  float peak = -1;
  double top = -1;
  for (uint32_t i = start; i < end; ++i) {
    if (m_Waveform[i] > peak) {
      peak = m_Waveform[i];
      top = (i - start) * m_Sampling;
    }
  }

  return top;
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
