#include "SiPMAdc.h"

#include <algorithm>
#include <math.h>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "SiPMRandom.h"

namespace sipm {

SiPMAdc::SiPMAdc(const uint32_t nbits, const double range, const double gain)
  : m_Nbits(nbits), m_Range(range), m_Gain(gain), m_Qlevels(pow(2, nbits)) {}

void SiPMAdc::setBandwidth(const double bw) {
  m_Bandwidth = bw;
  m_RC = 1 / (2 * M_PI * bw);
}

void SiPMAdc::lowpass(std::vector<double>& v, const double sampling) const {
  const double dt = 1e-9 * sampling;
  const double alpha = dt / (m_RC + dt);

  v[0] = alpha * v[0];
  for (uint32_t i = 1; i < v.size(); ++i) {
    v[i] = alpha * (v[i] - v[i - 1]) + v[i - 1];
  }
}

std::vector<int32_t> SiPMAdc::quantize(const std::vector<double>& v) const {
  std::vector<int32_t> out(v.size());

  const int32_t qlevels = m_Qlevels;
  const double gain = pow(10, (m_Gain / 20));
  const double width = m_Range / gain / qlevels;

  for (uint32_t i = 0; i < v.size(); ++i) { out[i] = v[i] / width; }
  std::replace_if(
    out.begin(), out.end(), [qlevels](int32_t x) { return x > qlevels; },
    qlevels);
  return out;
}

void SiPMAdc::jitter(std::vector<double>& lsignal, const double jit) const {
  std::vector<double> lsignalshift = lsignal; // Copy of signal
  double jitweight;

  const uint32_t n = lsignal.size();

  // Right shift
  if (jit > 0) {
    const uint32_t jitidx = std::floor(jit);
    jitweight = jit - jitidx;
    std::rotate(lsignal.rbegin(), lsignal.rbegin() + jitidx, lsignal.rend());
    // Signal shifted by 1 to right
    std::rotate(lsignalshift.rbegin(), lsignalshift.rbegin() + 1,
                lsignalshift.rend());
  }
  // Left shift
  else {
    const uint32_t jitidx = std::floor(-jit);
    jitweight = -jit - jitidx;
    std::rotate(lsignal.begin(), lsignal.begin() + jitidx, lsignal.end());
    // Signal shifted by 1 to left
    std::rotate(lsignalshift.begin(), lsignalshift.begin() + 1,
                lsignalshift.end());
  }
#ifdef __AVX2__
  const uint32_t last = n - n % 4;
  const __m256d __jitweight = _mm256_set1_pd(jitweight);
  __m256d __signal, __signalshift;
  for (uint32_t i = 0; i < last; i += 4) {
    __signal = _mm256_loadu_pd(&lsignal[i]);
    __signalshift = _mm256_loadu_pd(&lsignalshift[i]);
    __signal = _mm256_fmadd_pd(
      __jitweight, _mm256_sub_pd(__signalshift, __signal), __signal);
    _mm256_storeu_pd(&lsignal[i], __signal);
  }
  for (uint32_t i = last; i < n; ++i) {
    lsignal[i] += jitweight * (lsignalshift[i] - lsignal[i]);
  }
#else
  for (uint32_t i = 0; i < n; ++i) {
    // (1 - wight) * lsignal + weigth * lsignalshift
    lsignal[i] += jitweight * (lsignalshift[i] - lsignal[i]);
  }
#endif
}

SiPMDigitalSignal SiPMAdc::digitize(const SiPMAnalogSignal& signal) const {
  std::vector<double> lsignal = signal.waveform(); // Local analog signal

  if (m_Jitter > 0) {
    const double jit = rng.randGaussian(0, m_Jitter / signal.sampling());
    jitter(lsignal, jit);
  }

  if (m_Bandwidth > 0) { lowpass(lsignal, signal.sampling()); }

  // Quantize signal
  std::vector<int32_t> ldsignal = quantize(lsignal);

  // Generate digital signal object
  SiPMDigitalSignal dsignal(ldsignal, signal.sampling());

  return dsignal;
}

} // namespace sipm
