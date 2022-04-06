/** @class sipm::SiPMAnalogSignal SimSiPM/SimSiPM/SiPMAnalogSignal.h
 * SiPMAnalogSignal.h
 *
 *  @brief Class containing the waveform of the generated signal.
 *
 *  This class stores the generated signal as a SiPMVector<float>
 *  representing the sampled analog waveform.
 *  It also has some methods that can be used to extract some simple features
 *  from the signal.
 *
 *  The amplitude of the signal is scaled such that the signal from one photoelectron
 *  has height equal to 1 (not considering noise). In this way all other values like
 *  SNR and CCGV are scaled proportionally.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMSIGNAL_H
#define SIPM_SIPMSIGNAL_H

#include "SiPMMath.h"
#include "SiPMTypes.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdint.h>
#include <vector>

namespace sipm {
class SiPMAnalogSignal {
public:
  SiPMAnalogSignal() = default;

  SiPMAnalogSignal(const SiPMVector<float>& wav, const double sampling) noexcept
    : m_Waveform(wav), m_Sampling(sampling){};

  SiPMAnalogSignal& operator=(const SiPMVector<float>&& aVect) noexcept {
    m_Waveform = std::move(aVect);
    return *this;
  }

  inline float& operator[](const uint32_t i) noexcept { return m_Waveform[i]; }
  inline float operator[](const uint32_t i) const noexcept { return m_Waveform[i]; }

  /// @brief Returns the number of points in the waveform
  uint32_t size() const { return m_Waveform.size(); }
  /// @brief Resets the class to its initial state
  void clear() {
    m_Waveform.clear();
    m_peak = 0;
  }
  /// @brief Returns the sampling time of the signal in ns
  double sampling() const { return m_Sampling; }
  /// @brief Returns the waveform in an accessible data structure
  template <typename T = SiPMVector<float>> T waveform() const;

  /// @brief Returns integral of the signal
  double integral(const double, const double, const double) const;
  /// @brief Returns peak of the signal
  double peak(const double, const double, const double) const;
  /// @brief Returns time over threshold of the signal
  double tot(const double, const double, const double) const;
  /// @brief Returns time of arrival of the signal
  double toa(const double, const double, const double) const;
  /// @brief Returns time of peak
  double top(const double, const double, const double) const;

  /// @brief Sets the sampligng time of the signal
  // void setSampling(const double x) { m_Sampling = x; }

  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }
  friend std::ostream& operator<<(std::ostream&, const SiPMAnalogSignal&);

private:
  SiPMVector<float> m_Waveform;
  double m_Sampling = 1;
  mutable double m_peak = 0;
} /* SiPMAnalogSignal */;
} /* namespace sipm */
#endif /* SIPM_SIPMSIGNAL_H */
