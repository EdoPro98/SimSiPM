#include "SiPMAnalogSignal.h"

#include <algorithm>
#include <numeric>

namespace sipm {
const double SiPMAnalogSignal::integral(const double intstart,
                                        const double intgate,
                                        const double threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return std::accumulate(start, end, 0.0) * m_Sampling;
}

const double SiPMAnalogSignal::peak(const double intstart, const double intgate,
                                    const double threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return *std::max_element(start, end);
}

const double SiPMAnalogSignal::tot(const double intstart, const double intgate,
                                   const double threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  static const auto ifOver = [threshold](const double v) {
    return v > threshold;
  };

  return std::count_if(start, end, ifOver) * m_Sampling;
}

const double SiPMAnalogSignal::toa(const double intstart, const double intgate,
                                   const double threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  double toa = 0;

  for (auto it = start; it < end; ++it) {
    if (*it > threshold) {
      toa = (it - start) * m_Sampling;
      break;
    }
  }
  return toa;
}

const double SiPMAnalogSignal::top(const double intstart, const double intgate,
                                   const double threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return (std::max_element(start, end) - start) * m_Sampling;
}

} // namespace sipm
