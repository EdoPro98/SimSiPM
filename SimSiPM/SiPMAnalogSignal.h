#include <vector>
#include <stdint.h>

#ifndef SIPM_SIPMSIGNAL_H
#define SIPM_SIPMSIGNAL_H

namespace sipm{

class SiPMAnalogSignal{
public:
  // Move assignement
  SiPMAnalogSignal& operator=(const std::vector<double>&& aVect){
    m_Waveform = std::move(aVect);
    return *this;
  };
  //Copy assignement
  SiPMAnalogSignal& operator=(const std::vector<double>& aVect){
    m_Waveform = aVect;
    return *this;
  };

  // Access to array
  double& operator[](const uint32_t i){return m_Waveform[i];}
  const double& operator[](const uint32_t i)const{return m_Waveform[i];}

  const std::vector<double>& waveform()const{return m_Waveform;}
  const double sampling()const{return m_Sampling;}

  const double integral(const double, const double, const double)const;
  const double peak(const double, const double, const double)const;
  const double tot(const double, const double, const double)const;
  const double toa(const double, const double, const double)const;
  const double top(const double, const double, const double)const;
  const uint32_t size()const{return m_Waveform.size();}

  void setSampling(double x){m_Sampling = x;}

private:
  std::vector<double> m_Waveform;
  double m_Sampling;
};

} /* NAMESPACE_SIPM */
#endif /* SIPM_SIPMSIGNAL_H */
