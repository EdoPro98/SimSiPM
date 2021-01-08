#include <stdint.h>

#ifndef SIPM_SIPMHITS_H
#define SIPM_SIPMHITS_H

namespace sipm {

class SiPMHit {
public:
  enum class HitType {
    kPhotoelectron,
    kDarkCount,
    kOpticalCrosstalk,
    kAfterPulse
  };

  SiPMHit(double, double, uint32_t, uint32_t, const HitType&) noexcept;
  const bool operator<(const SiPMHit& aHit) const noexcept {
    return m_Time < aHit.m_Time;
  }

  // Getters
  const double time() const { return m_Time; }
  const uint32_t row() const { return m_Row; }
  const uint32_t col() const { return m_Col; }
  const uint32_t id() const { return m_Id; }
  const double amplitude() const { return m_Amplitude; }
  double& amplitude() { return m_Amplitude; }
  const HitType& hitType() const { return m_HitType; }

private:
  static const uint32_t makePair(const uint32_t x, const uint32_t y) {
    return ((x + y + 1) * (x + y) << 1) + y;
  }

  uint32_t m_Row, m_Col;
  uint32_t m_Id;
  double m_Time;
  double m_Amplitude;
  HitType m_HitType;
};

} // namespace sipm
#endif /* SIPM_SIPMHITS_H */
