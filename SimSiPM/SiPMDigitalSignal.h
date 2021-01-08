#include <stdint.h>
#include <vector>

#ifndef SIPM_SIPMDIGITALSIGNAL_H
#define SIPM_SIPMDIGITALSIGNAL_H

namespace sipm {

class SiPMDigitalSignal {
public:
  // Empty signal constructor
  SiPMDigitalSignal(const double sampling) noexcept : m_Sampling(sampling){};

  // Sgnal constructor
  SiPMDigitalSignal(const std::vector<int32_t>& wav,
                    const double sampling) noexcept
    : m_Waveform(wav), m_Sampling(sampling){};

  // Move assignement from vector
  SiPMDigitalSignal& operator=(const std::vector<int32_t>&& aVect) noexcept {
    m_Waveform = std::move(aVect);
    return *this;
  };
  // Copy assignement from vector
  SiPMDigitalSignal& operator=(const std::vector<int32_t>& aVect) noexcept {
    m_Waveform = aVect;
    return *this;
  };

  // Access to signal elements
  int32_t& operator[](const uint32_t i) noexcept { return m_Waveform[i]; }
  const int32_t& operator[](const uint32_t i) const noexcept {
    return m_Waveform[i];
  }

  // Access to waveform
  const std::vector<int32_t>& waveform() const { return m_Waveform; }

  const int32_t integral(const double, const double, const int32_t) const;
  const int32_t peak(const double, const double, const int32_t) const;
  const double tot(const double, const double, const int32_t) const;
  const double toa(const double, const double, const int32_t) const;
  const double top(const double, const double, const int32_t) const;

private:
  std::vector<int32_t> m_Waveform;
  const double m_Sampling;
};

} // namespace sipm
#endif /* SIPM_SIPMSIGNAL_H */
