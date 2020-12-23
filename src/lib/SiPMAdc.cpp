#include "SiPMAdc.h"
#include <math.h>

namespace sipm{

SiPMAdc::SiPMAdc(uint32_t nbits, double range, double offset, double gain):
  m_Nbits(nbits),
  m_Range(range),
  m_Offset(offset),
  m_Gain(gain),
  m_Qlevels(pow(2,nbits))
{}


SiPMDigitalSignal SiPMAdc::digitize(SiPMAnalogSignal& signal)const{
  std::vector<int32_t> ldsignal(signal.size(),0);
  std::vector<double> lsignal = signal.waveform();

  const double gain = pow(10,(m_Gain/20));
  const double max = m_Range / gain;
  const double width = m_Range / m_Qlevels;

  if (m_HasBandwidth == true){
    // Simple lowpass filter
    const double RC = 1 / (2*M_PI*m_BandWidth);
    const double alpha = signal.sampling() / (RC + signal.sampling());
    lsignal[0] = alpha * lsignal[0];
    for(uint32_t i=1; i<lsignal.size(); ++i){
      lsignal[i] = alpha * lsignal[i] + (1-alpha) * lsignal[i-1];
    }
  }

  if (m_HasJitter == true){
    // Apply time jitter
  }

  // Quantize signal
  for (uint32_t i=0; i<lsignal.size(); ++i){
    ldsignal[i] = (lsignal[i] + m_Offset) / width;
  }

  // Replace values out of range
  const uint32_t lQlevels = m_Qlevels;
  std::replace_if(ldsignal.begin(), ldsignal.end(),[lQlevels](uint32_t x){return x > lQlevels;},lQlevels);

  // Generate digital signal object
  SiPMDigitalSignal dsignal(signal.size(),signal.sampling());
  dsignal = ldsignal;

  return dsignal;
}

}
