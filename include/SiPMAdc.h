/** @class sipm::SiPMAdc SimSiPM/SimSiPM/SiPMAdc.h SiPMAdc.h
 *
 *  @brief Class that simulates the operation of an ADC converter.
 *
 *  This class is used to convert a @ref SiPMAnalogSignal into a @ref
 *  SiPMDigitalSignal. The signal is quantized using a given number of bits
 *  after it is amplified using a given gain.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMADC_H
#define SIPM_SIPMADC_H

#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <vector>

#include "SiPMAnalogSignal.h"
#include "SiPMDigitalSignal.h"
#include "SiPMRandom.h"

namespace sipm {

class SiPMAdc {
public:
  /// @brief SiPMAdc default constructor
  SiPMAdc() = default;
  /// @brief SiPMAdc contructor with given parameters
  SiPMAdc(const uint32_t, const double, const double);

  uint32_t nBits() const { return m_Nbits; }
  double range() const { return m_Range; }
  double gain() const { return m_Gain; }
  double jitter() const { return m_Jitter; }

  /// @brief Digitizes an analog signalt to a digital one
  SiPMDigitalSignal digitize(const SiPMAnalogSignal&) const;

  /// @brief Sets jitter parameter
  void setJitter(const double jit) { m_Jitter = jit; }
  /// @brief Sets range parameter
  void setRange(const double rng) { m_Range = rng; }
  /// @brief Sets gain parameter
  void setGain(const double gn) { m_Gain = gn; }
  /// @brief Sets number of bits
  void setBits(const double bts) { m_Nbits = bts; }
  // @ Turns off jitter effect
  void setJitterOff() { m_Jitter = 0; }

private:
  /// @brief Quantizes a signal using a given number of bits
  std::vector<int32_t> quantize(const std::vector<double>&, uint32_t, double, double) const __attribute__((hot));
  /// @brief Adds jitter to a signal
  /// @todo Maybe better to return by reference here
  std::vector<double> addJitter(std::vector<double>&, const double) const __attribute__((hot));

  uint32_t m_Nbits;
  double m_Range;
  double m_Gain;

  double m_Jitter = 0;

  mutable SiPMRandom rng;
}; /* SiPMAdc */
} // namespace sipm
#endif /* SIPM_SIPMADC_H */
