/** @class sipm::SiPMAnalogSignal SiPMAnalogSignal.h
 *
 *  @brief Class containing the waveform of the generated signal.
 *
 *  This class stores the generated signal as a std::vector<float>
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

#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

namespace sipm {
class SiPMAnalogSignal {
public:
  /// @brief Creates an empty signal
  SiPMAnalogSignal() = default;

  /// @brief Creates a signal from a waveform
  /// @param wav Waveform samples (1 = amplitude of one photoelectron)
  /// @param sampling Sampling time in ns
  SiPMAnalogSignal(std::vector<float> wav, double sampling) noexcept
    : m_Waveform(std::move(wav)), m_Sampling(sampling) {};

  /// @brief Direct access to the waveform samples
  /// @return Pointer to the first sample, valid until the signal is modified
  float* data() noexcept { return m_Waveform.data(); }

  /// @brief Access to the i-th sample, no bounds check
  /// @param i Sample index, must be smaller than size()
  /// @return Reference to the sample
  inline float& operator[](const uint32_t i) noexcept { return m_Waveform[i]; }
  /// @brief Access to the i-th sample, no bounds check
  /// @param i Sample index, must be smaller than size()
  /// @return Value of the sample
  inline float operator[](const uint32_t i) const noexcept { return m_Waveform[i]; }

  /// @brief Number of points in the waveform
  /// @return Number of samples
  inline uint32_t size() const { return m_Waveform.size(); }
  /// @brief Sampling time of the signal
  /// @return Sampling time in ns
  inline double sampling() const { return m_Sampling; }
  /// @brief Length of the signal
  /// @return Signal length in ns (size() * sampling())
  inline double length() const { return (double)m_Waveform.size() / m_Sampling; }
  /// @brief Waveform samples
  /// @return Const reference to the samples (1 = amplitude of one photoelectron)
  inline const std::vector<float>& waveform() const noexcept {return m_Waveform; }


  /// @brief Integral of the signal in an integration gate
  double integral(const double, const double, const double) const;
  /// @brief Returns peak of the signal
  double peak(const double, const double, const double) const;
  /// @brief Returns time over threshold of the signal
  double tot(const double, const double, const double) const;
  /// @brief Returns time of arrival of the signal
  double toa(const double, const double, const double) const;
  /// @brief Returns time of peak
  double top(const double, const double, const double) const;

  /// @brief Description of the signal
  /// @return String printed by operator<<
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }
  /// @brief Prints length, sampling time and number of points of the signal
  friend std::ostream& operator<<(std::ostream&, const SiPMAnalogSignal&);

private:
  std::vector<float> m_Waveform;  ///< Waveform samples
  double m_Sampling;  ///< Sampling time in ns
} /* SiPMAnalogSignal */;

} /* namespace sipm */
#endif /* SIPM_SIPMSIGNAL_H */
