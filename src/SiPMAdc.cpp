#include "SiPMAdc.h"

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "SiPMRandom.h"

namespace sipm {

SiPMAdc::SiPMAdc(const uint32_t nbits, const double range, const double gain)
  : m_Nbits(nbits), m_Range(range), m_Gain(gain) {}

/**
 * @param v Vector to quantize
 * @param nbits Number of bits to use for quantization
 * @param range Range to use for the quantization [-range,+range]
 * @param gain Gain in dB to apply before quantization
 * @return Quantized input vector
 */
std::vector<int32_t> SiPMAdc::quantize(const std::vector<double>& v, const uint32_t nbits, const double range,
                                       const double gain) const {
  std::vector<int32_t> out(v.size());

  const double qlevels = pow(2, nbits);
  const double gainlinear = pow(10, (gain / 20));
  const double reciprocal_width = 1 / (range / gainlinear / qlevels);

  for (uint32_t i = 0; i < v.size(); ++i) {
    out[i] = v[i] * reciprocal_width;
  }
  std::replace_if(
    out.begin(), out.end(), [qlevels](int32_t x) { return x > qlevels; }, qlevels);
  return out;
}

/**
 * @param signal Input signal to apply jitter to
 * @param jit Jitter value to apply
 * @return Signal with jitter applied
 */
std::vector<double> SiPMAdc::addJitter(std::vector<double>& signal, const double jit) const {
  const uint32_t n = signal.size();
  std::vector<double> lsignalshift = signal; // Copy of signal
  double jitweight;

  // Right shift (jitter is positive -> need to move signal "to right")
  if (jit > 0) {
    const uint32_t jitidx = floor(jit);
    jitweight = jit - jitidx;
    std::rotate(signal.rbegin(), signal.rbegin() + jitidx, signal.rend());
    // Signal shifted by 1 idx to right
    std::rotate(lsignalshift.rbegin(), lsignalshift.rbegin() + 1, lsignalshift.rend());
  }
  // Left shift (jitter is negative -> need to move signal "to left")
  else {
    const uint32_t jitidx = floor(-jit);
    jitweight = -jit - jitidx;
    std::rotate(signal.begin(), signal.begin() + jitidx, signal.end());
    // Signal shifted by 1 to left
    std::rotate(lsignalshift.begin(), lsignalshift.begin() + 1, lsignalshift.end());
  }

  // In this part I take into account of a signal shift smaller thant 1 idx.
  // In this case the resulting signal is the wighted sum of the original signal
  // and the signal shiftedby one.
#ifdef __AVX2__
  const __m256d __jitweight = _mm256_set1_pd(jitweight);
  __m256d __signal, __signalshift;
  for (uint32_t i = 0; i < n - 4; i += 4) {
    __signal = _mm256_loadu_pd(&signal[i]);
    __signalshift = _mm256_loadu_pd(&lsignalshift[i]);
    __signal = _mm256_fmadd_pd(__jitweight, _mm256_sub_pd(__signalshift, __signal), __signal);
    _mm256_storeu_pd(&signal[i], __signal);
  }
  for (uint32_t i = n - 4; i < n; ++i) {
    signal[i] += jitweight * (lsignalshift[i] - signal[i]);
  }
#else
  for (uint32_t i = 0; i < n; ++i) {
    // (1 - weigth) * lsignal + weigth * lsignalshift
    signal[i] += jitweight * (lsignalshift[i] - signal[i]);
  }
#endif
  return signal;
}

/**
 * @param signal Input signal to be digitized
 * @returns Digitized SiPMAnalogSignal
 */
SiPMDigitalSignal SiPMAdc::digitize(const SiPMAnalogSignal& signal) const {
  std::vector<double> lsignal = signal.waveform(); // Local copy of analog signal
  const double sampling = signal.sampling();

  if (m_Jitter > 0) {
    const double jit = rng.randGaussian(0, m_Jitter / sampling);
    lsignal = addJitter(lsignal, jit);
  }

  // Quantize signal
  std::vector<int32_t> ldsignal = quantize(lsignal, m_Nbits, m_Range, m_Gain);

  // Generate digital signal object
  SiPMDigitalSignal dsignal(ldsignal, sampling);

  return dsignal;
}
} // namespace sipm
