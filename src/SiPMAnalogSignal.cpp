#include "SiPMAnalogSignal.h"
#include <cstdint>
#include <iomanip>

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
  const size_t startIdx = static_cast<size_t>(intstart / m_Sampling);
  const size_t endIdx = static_cast<size_t>((intstart + intgate) / m_Sampling);

  bool isOver = false;
  double integral = 0;

  for (size_t i = startIdx; i < endIdx; ++i) {
      const float sample = m_Waveform[i];
      if (sample > threshold) {
          isOver = true;
      }
      integral += sample;
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
  const size_t startIdx = static_cast<size_t>(intstart / m_Sampling);
  const size_t endIdx = static_cast<size_t>((intstart + intgate) / m_Sampling);

  float peak = -1.0;

  for (size_t i = startIdx; i < endIdx; ++i) {
      const float val = m_Waveform[i];
      if (val > threshold && val > peak) {
          peak = val;
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
  auto start = m_Waveform.begin() + intstart / m_Sampling;
  const auto end = m_Waveform.cbegin() + (intstart + intgate) / m_Sampling;

  double tot = 0;
  while (start < end) {
    if (*start++ > threshold) {
      tot++;
    }
  }
  return tot > 0 ? tot * m_Sampling : -1;
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
  const uint32_t start = intstart / m_Sampling;
  const uint32_t end = (intstart + intgate) / m_Sampling;

  for (uint32_t i = start; i < end - 1; ++i) {
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
  const uint32_t start = intstart / m_Sampling;
  const uint32_t end = (intstart + intgate) / m_Sampling;
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
