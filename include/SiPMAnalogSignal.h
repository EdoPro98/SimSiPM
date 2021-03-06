/** @class sipm::SiPMAnalogSignal SimSiPM/SimSiPM/SiPMAnalogSignal.h
 * SiPMAnalogSignal.h
 *
 *  @brief Class containing the waveform of the generated signal.
 *
 *  This class stores the generated signal as a std::vector<double>
 *  representing the sampled analog waveform.
 *  It also has some methods that can be used to extract some simple features
 *  from the signal.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */
#include <stdint.h>
#include <vector>

#ifndef SIPM_SIPMSIGNAL_H
#define SIPM_SIPMSIGNAL_H

namespace sipm {

class SiPMAnalogSignal {
public:
  /// @brief SiPMAnalogSignal default constructor
  SiPMAnalogSignal() = default;

  /// @brief SiPMAnalogSignal constructor from a std::vector
  SiPMAnalogSignal(const std::vector<double>& wav, const double sampling) noexcept
      : m_Waveform(wav), m_Sampling(sampling){};

  /// @brief Move assignement operator from a std::vector
  SiPMAnalogSignal& operator=(const std::vector<double>&& aVect) noexcept {
    m_Waveform = std::move(aVect);
    return *this;
  };

  /// @brief Copy assignement operator from a std::vector
  SiPMAnalogSignal& operator=(const std::vector<double>& aVect) noexcept {
    m_Waveform = aVect;
    return *this;
  };

  /// @brief Used to access signal elements as if it is a std::vector
  double& operator[](const uint32_t i) noexcept { return m_Waveform[i]; }
  /// @brief Used to access signal elements as if it is a std::vector
  const double& operator[](const uint32_t i) const noexcept { return m_Waveform[i]; }

  /// @brief Returns the size of the vector containing the signal
  const uint32_t size() const { return m_Waveform.size(); }
  /// @brief Clears all elements of the vector containing the signal
  const void clear() { return m_Waveform.clear(); }
  /// @brief Returns the sampling time of the signal in ns
  const double sampling() const { return m_Sampling; }
  /// @brief Returns a std::vector containing the sampled waveform
  const std::vector<double>& waveform() const { return m_Waveform; }

  /// @brief Returns integral of the signal
  const double integral(const double, const double, const double) const;
  /// @brief Returns peak of the signal
  const double peak(const double, const double, const double) const;
  /// @brief Returns time over threshold of the signal
  const double tot(const double, const double, const double) const;
  /// @brief Returns time of arrival of the signal
  const double toa(const double, const double, const double) const;
  /// @brief Returns time of peak
  const double top(const double, const double, const double) const;

  /// @brief Sets the sampligng time of the signal
  void setSampling(const double x) { m_Sampling = x; }

  /// @brief Applies a low-pass filter to the input vector
  SiPMAnalogSignal lowpass(const double) const __attribute__((hot));

private:
  std::vector<double> m_Waveform;
  double m_Sampling;

} /* SiPMAnalogSignal */;
} /* namespace sipm */
#endif /* SIPM_SIPMSIGNAL_H */
