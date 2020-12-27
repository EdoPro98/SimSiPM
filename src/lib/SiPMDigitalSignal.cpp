#include "SiPMDigitalSignal.h"

#include <numeric>
#include <algorithm>

namespace sipm{
const int32_t SiPMDigitalSignal::integral(const double intstart, const double intgate,
  const int32_t threshold)const{

  const uint32_t peThreshold = m_Dpp * threshold;
  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  if (*std::max_element(start, end) > peThreshold){
    return std::accumulate(start, end, static_cast<int32_t>(0)) * m_Sampling;
  } else {
    return -1;
  }
}


const int32_t SiPMDigitalSignal::peak(const double intstart, const double intgate,
  const int32_t threshold)const{

  const uint32_t peThreshold = m_Dpp * threshold;
  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  const int32_t peak = *std::max_element(start, end);
  if (peak > peThreshold){
    return peak;
  } else {
    return -1;
  }
}


const double SiPMDigitalSignal::tot(const double intstart, const double intgate,
  const int32_t threshold)const{

  const uint32_t peThreshold = m_Dpp * threshold;
  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  if (*std::max_element(start, end) > peThreshold){
    return std::count_if(start, end, [peThreshold](const double v){return v > peThreshold;}) * m_Sampling;
  } else {
    return -1;
  }
}


const double SiPMDigitalSignal::toa(const double intstart, const double intgate,
  const int32_t threshold)const{

  const uint32_t peThreshold = m_Dpp * threshold;
  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  double toa = -1;

  for(auto it = start; it < end; ++it){
    if(*it > peThreshold){
      toa = (it - start) * m_Sampling;
      break;
    }
  }
  return toa;
}


const double SiPMDigitalSignal::top(const double intstart, const double intgate,
  const int32_t threshold)const{

  const uint32_t peThreshold = m_Dpp * threshold;
  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  if (*std::max_element(start, end) > peThreshold){
    return (std::max_element(start, end) - start) * m_Sampling;
  } else {
    return -1;
  }
}

} /* NAMESPACE_SIPM */
