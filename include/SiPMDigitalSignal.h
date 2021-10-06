/** @class sipm::SiPMDigitalSignal SimSiPM/SimSiPM/SiPMDigitalSignal.h
 * SiPMDigitalSignal.h
 *
 *  @brief Class containing the digitized waveform of the generated signal.
 *
 *  This class stores the generated signal as a std::vector<int32_t>
 *  representing the sampled analog waveform passed through an ADC.
 *  It also has some methods that can be used to extract some simple features
 *  from the signal.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <vector>

#ifndef SIPM_SIPMDIGITALSIGNAL_H
#define SIPM_SIPMDIGITALSIGNAL_H

namespace sipm {

class SiPMDigitalSignal {
public:
  /// @brief SiPMDigitalSignal default constructor
  SiPMDigitalSignal(const double sampling) noexcept : m_Sampling(sampling){};

  /// @brief SiPMDigitalSignal constructor from a std::vector
  SiPMDigitalSignal(const std::vector<int32_t>& wav, const double sampling) noexcept
    : m_Waveform(wav), m_Sampling(sampling){};

  /// @brief Move assignement operator from a std::vector
  SiPMDigitalSignal& operator=(const std::vector<int32_t>&& aVect) noexcept {
    m_Waveform = std::move(aVect);
    return *this;
  };

  /// @brief Copy assignement operator from a std::vector
  inline int32_t& operator[](const uint32_t i) noexcept { return m_Waveform[i]; }
  inline int32_t operator[](const uint32_t i) const noexcept { return m_Waveform[i]; }

  /// @brief Returns the size of the vector containing the signal
  uint32_t size() const { return m_Waveform.size(); }
  /// @brief Clears all elements of the vector containing the signal
  void clear() { return m_Waveform.clear(); }
  /// @brief Returns the sampling time of the signal in ns
  double sampling() const { return m_Sampling; }
  /// @brief Used to access signal elements as if it is a std::vector
  const std::vector<int32_t> waveform() const { return m_Waveform; }

  /// @brief Returns integral of the signal
  int32_t integral(const double, const double, const int32_t) const;
  /// @brief Returns peak of the signal
  int32_t peak(const double, const double, const int32_t) const;
  /// @brief Returns time over threshold of the signal
  double tot(const double, const double, const int32_t) const;
  /// @brief Returns time of arrival of the signal
  double toa(const double, const double, const int32_t) const;
  /// @brief Returns time of peak
  double top(const double, const double, const int32_t) const;

  friend std::ostream& operator<< (std::ostream&, const SiPMDigitalSignal&);

private:
  std::vector<int32_t> m_Waveform;
  const double m_Sampling;
};

} // namespace sipm
#endif /* SIPM_SIPMSIGNAL_H */
