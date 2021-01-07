#include "SiPMDigitalSignal.h"

#include <algorithm>
#include <numeric>

namespace sipm {
const int32_t SiPMDigitalSignal::integral(const double intstart,
                                          const double intgate,
                                          const int32_t threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  if (*std::max_element(start, end) > threshold) {
    return std::accumulate(start, end, static_cast<int32_t>(0)) * m_Sampling;
  } else {
    return -1;
  }
}

const int32_t SiPMDigitalSignal::peak(const double intstart,
                                      const double intgate,
                                      const int32_t threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  const int32_t peak = *std::max_element(start, end);
  if (peak > threshold) {
    return peak;
  } else {
    return -1;
  }
}

const double SiPMDigitalSignal::tot(const double intstart, const double intgate,
                                    const int32_t threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  if (*std::max_element(start, end) > threshold) {
    return std::count_if(
               start, end,
               [threshold](const double v) { return v > threshold; }) *
           m_Sampling;
  } else {
    return -1;
  }
}

const double SiPMDigitalSignal::toa(const double intstart, const double intgate,
                                    const int32_t threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  double toa = -1;

  for (auto it = start; it < end; ++it) {
    if (*it > threshold) {
      toa = (it - start) * m_Sampling;
      break;
    }
  }
  return toa;
}

const double SiPMDigitalSignal::top(const double intstart, const double intgate,
                                    const int32_t threshold) const {

  const auto start =
      m_Waveform.begin() + static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  if (*std::max_element(start, end) > threshold) {
    return (std::max_element(start, end) - start) * m_Sampling;
  } else {
    return -1;
  }
}

} // namespace sipm
