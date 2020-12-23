#include <SiPM.h>
#include "SiPMDigitalSignal.h"

#ifndef SIPM_SIPMADC_H
#define SIPM_SIPMADC_H

namespace sipm{
class SiPMAdc{
public:
  SiPMAdc(uint32_t , double, double, double);

  SiPMDigitalSignal digitize(SiPMAnalogSignal&)const;
  void setBandWidth(double bw){m_BandWidth = bw; m_HasBandwidth = true;}
  void setJitter(double jit){m_Jitter = jit; m_HasJitter = true;}

private:
  const uint32_t m_Nbits;
  const uint32_t m_Qlevels;
  const double m_Range;
  const double m_Offset;
  const double m_Gain;

  double m_BandWidth;
  double m_Jitter;

  bool m_HasBandwidth = false;
  bool m_HasJitter = false;


}; /* SiPMAdc */
}
#endif /* SIPM_SIPMADC_H */
