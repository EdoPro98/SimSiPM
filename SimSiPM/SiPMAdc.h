#include "SiPM.h"
#include "SiPMDigitalSignal.h"

#ifndef SIPM_SIPMADC_H
#define SIPM_SIPMADC_H

namespace sipm{
class SiPMAdc{
public:
  SiPMAdc(const uint32_t, const double, const double);

  SiPMDigitalSignal digitize(const SiPMAnalogSignal&)const;
  void setJitter(const double jit){m_Jitter = jit;}
  void setBandwidth(const double);

private:
  template <typename T>
  static void lowpass(std::vector<T>&, const double);
  template <typename T>
  static void quantize(const std::vector<T>&, std::vector<int32_t>&, const double);

  const uint32_t m_Nbits;
  const uint32_t m_Qlevels;
  const double m_Range;
  const double m_Gain;

  double m_Jitter = 0;
  double m_Bandwidth = 0;
  double m_RC;
}; /* SiPMAdc */
}
#endif /* SIPM_SIPMADC_H */
