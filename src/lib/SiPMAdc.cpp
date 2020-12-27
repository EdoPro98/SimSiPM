#include "SiPMAdc.h"
#include "SiPMRandom.h"
#include <math.h>

#include <iostream>

namespace sipm{

SiPMAdc::SiPMAdc(const uint32_t nbits, const double range, const double gain):
  m_Nbits(nbits),
  m_Range(range),
  m_Gain(gain),
  m_Qlevels(pow(2,nbits))
{}


void SiPMAdc::setBandwidth(const double bw){
  m_Bandwidth = bw;
  m_RC = 1 / (2*M_PI*bw);
}


template <typename T>
void SiPMAdc::lowpass(std::vector<T>& v, const double alpha){
  v[0] = alpha * v[0];
  for(uint32_t i = 0; i < v.size(); ++i){
    v[i] = alpha * v[i] + (1-alpha) * v[i-1];
  }
}


template <typename T>
void SiPMAdc::quantize(const std::vector<T>& v, std::vector<int32_t>& d, const double width){
  for(uint32_t i = 0; i < v.size(); ++i){
    d[i] = v[i] / width;
  }
}


SiPMDigitalSignal SiPMAdc::digitize(const SiPMAnalogSignal& signal)const{
  std::vector<int32_t> ldsignal(signal.size(),0);   // Local digital signal
  std::vector<double> lsignal = signal.waveform();  // Local analog signal

  const double gain = pow(10,(m_Gain/20));
  const double width = m_Range / gain / m_Qlevels;
  const uint32_t dpp = 1 / width;

  if (m_Jitter > 0){
    const double jit = randGaussian(0, m_Jitter/signal.sampling());
    double jitweight;
    std::vector<double> lsignalshift = lsignal;
    // Right shift
    if (jit > 0){
      const uint32_t jitidx = std::floor(jit);
      jitweight = jit - jitidx;
      std::rotate(lsignal.rbegin(), lsignal.rbegin() + jitidx, lsignal.rend());
      std::rotate(lsignalshift.rbegin(), lsignalshift.rbegin() + 1, lsignalshift.rend());
    }
    // Left shift
    else{
      const uint32_t jitidx = std::floor(-jit);
      jitweight = - jit - jitidx;
      std::rotate(lsignal.begin(), lsignal.begin() + jitidx, lsignal.end());
      std::rotate(lsignalshift.begin(), lsignalshift.begin() + jitidx, lsignalshift.end());
    }

    for(uint32_t i=0; i<lsignal.size();++i){
      lsignal[i] = (1 - jitweight) * lsignal[i] + jitweight * lsignalshift[i];
    }

  }

  if (m_Bandwidth > 0){
    const double dt = 1e-9 * signal.sampling();
    const double alpha = dt / (m_RC + dt);
    lowpass(lsignal, alpha);
  }


  // Quantize signal
  quantize(lsignal, ldsignal, width);

  // Replace values out of range
  const uint32_t lQlevels = m_Qlevels;
  std::replace_if(ldsignal.begin(), ldsignal.end(),[lQlevels](uint32_t x){return x > lQlevels;},lQlevels);

  // Generate digital signal object
  SiPMDigitalSignal dsignal(signal.sampling(), dpp);
  dsignal = ldsignal;

  return dsignal;
}

}
