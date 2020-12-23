#include <vector>
#include <stdint.h>

#ifndef SIPM_SIPMDIGITALSIGNAL_H
#define SIPM_SIPMDIGITALSIGNAL_H

namespace sipm{

class SiPMDigitalSignal{
public:

  SiPMDigitalSignal(uint32_t pts, double sampling) :
    m_SignalPoints(pts),
    m_Sampling(sampling)
  {};
  // Move assignement
  SiPMDigitalSignal& operator=(const std::vector<int32_t>&& aVect){
    m_Waveform = std::move(aVect);
    return *this;
  };
  //Copy assignement
  SiPMDigitalSignal& operator=(const std::vector<int32_t>& aVect){
    m_Waveform = aVect;
    return *this;
  };

  // Access to array
  int32_t& operator[](const uint32_t i){return m_Waveform[i];}
  const int32_t& operator[](const uint32_t i)const{return m_Waveform[i];}

  const std::vector<int32_t>& waveform()const{return m_Waveform;}

  const int32_t integral(const double, const double, const int32_t)const;
  const int32_t peak(const double, const double, const int32_t)const;
  const double tot(const double, const double, const int32_t)const;
  const double toa(const double, const double, const int32_t)const;
  const double top(const double, const double, const int32_t)const;

private:
  std::vector<int32_t> m_Waveform;
  const double m_Sampling;
  const uint32_t m_SignalPoints;
};

} /* NAMESPACE_SIPM */
#endif /* SIPM_SIPMSIGNAL_H */
