#include "SiPM.h"
#include "SiPMDigitalSignal.h"

#ifndef SIPM_SIPMADC_H
#define SIPM_SIPMADC_H

namespace sipm{
class SiPMAdc{
public:
  SiPMAdc() = default;
  SiPMAdc(const uint32_t, const double, const double);

  SiPMDigitalSignal digitize(const SiPMAnalogSignal&)const;

  void setJitter(const double jit){m_Jitter = jit;}
  void setBandwidth(const double);

private:
  void lowpass(std::vector<double>&, const double)const;
  std::vector<int32_t> quantize(const std::vector<double>&)const;
  void jitter(std::vector<double>&, const double)const;

  uint32_t m_Nbits;
  uint32_t m_Qlevels;
  double m_Range;
  double m_Gain;

  double m_Jitter = 0;
  double m_Bandwidth = 0;
  double m_RC;
}; /* SiPMAdc */
}
#endif /* SIPM_SIPMADC_H */
