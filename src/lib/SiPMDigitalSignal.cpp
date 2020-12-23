#include "SiPMDigitalSignal.h"

#include <numeric>
#include <algorithm>

namespace sipm{
const int32_t SiPMDigitalSignal::integral(const double intstart, const double intgate,
  const int32_t threshold)const{

  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return std::accumulate(start, end, static_cast<int32_t>(0)) * m_Sampling;
}


const int32_t SiPMDigitalSignal::peak(const double intstart, const double intgate,
  const int32_t threshold)const{

  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return *std::max_element(start, end);
}


const double SiPMDigitalSignal::tot(const double intstart, const double intgate,
  const int32_t threshold)const{

  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return std::count_if(start, end, [threshold](const double v){return v > threshold;}) * m_Sampling;
}


const double SiPMDigitalSignal::toa(const double intstart, const double intgate,
  const int32_t threshold)const{

  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);
  double toa = 0;

  for(auto it = start; it < end; ++it){
    if(*it > threshold){
      toa = (it - start) * m_Sampling;
      break;
    }
  }
  return toa;
}


const double SiPMDigitalSignal::top(const double intstart, const double intgate,
  const int32_t threshold)const{

  const auto start = m_Waveform.begin() +
   static_cast<uint32_t>(intstart / m_Sampling);
  const auto end = start + static_cast<uint32_t>(intgate / m_Sampling);

  return (std::max_element(start, end) - start) * m_Sampling;
}

} /* NAMESPACE_SIPM */
