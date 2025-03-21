/** @class sipm::SiPMHit SimSiPM/SimSiPM/SiPMHit.h SiPMHit.h
 *
 * @brief Class storing informations relative to a single SiPM hitted cell.
 *
 * This class is used mainly to store informations relative to a single hit on a
 * SiPM cell. Informations stored in thiss class will be used to generate the
 * signal for each SiPM cell.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMHITS_H
#define SIPM_SIPMHITS_H

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

namespace sipm {
class SiPMHit {
public:
  /** @enum HitType
   * Defines the generating process for the hit
   */
  enum class HitType : uint8_t {
    kPhotoelectron,           ///< Hit generated by a photoelectron
    kDarkCount,               ///< Hit generated by a dark count event
    kOpticalCrosstalk,        ///< Hit generated by an optical crosstalk
    kDelayedOpticalCrosstalk, ///< Hit generated by a delayed optical crosstalk
    kFastAfterPulse,          ///< Hit generated by a fast afterpulse
    kSlowAfterPulse           ///< Hit generated by a slow afterpulse
  };

  constexpr SiPMHit(double time, float amp, uint32_t row, uint32_t col, HitType type,
                    const SiPMHit* parentPtr = nullptr) noexcept
    : m_Time(time), m_Amplitude(amp), m_Row(row), m_Col(col), m_HitType(type), m_ParentPtr(parentPtr) {}


  /// @brief Comparison operator for hits
  /**
   * Hits are ordered based on their time:
   * @f[Hit_1 < Hit_2 \Leftrightarrow Hit_1.time < Hit_2.time @f]
   * Sorting a vector of hits gives the chronological order of hits by
   * arrving/generating time.
   */
  constexpr bool operator<(const SiPMHit& rhs) const noexcept { return m_Time < rhs.m_Time; }
  constexpr bool operator<=(const SiPMHit& rhs) const noexcept { return !(rhs < *this); }
  constexpr bool operator>(const SiPMHit& rhs) const noexcept { return rhs < *this; }
  constexpr bool operator>=(const SiPMHit& rhs) const noexcept { return !(*this < rhs); }

  /// @brief Operator used to check if the hit is generated in the same cell
  /**
   * Hits are considered equal if they have same row and column,
   * hence the same SiPM cell
   */
  constexpr bool operator==(const SiPMHit& rhs) const noexcept { return m_Row == rhs.m_Row && m_Col == rhs.m_Col; }

  /// @brief Returns hit time
  constexpr double time() const noexcept { return m_Time; }
  /// @brief Returns row of hitted cell
  constexpr uint32_t row() const noexcept { return m_Row; }
  /// @brief Returns column of hitted cell
  constexpr uint32_t col() const noexcept { return m_Col; }
  /// @brief Returns amplitude of the signal produced by the hit
  float amplitude() const noexcept { return m_Amplitude; }
  float& amplitude() { return m_Amplitude; }
  /// @brief Returns hit type to identify the hits
  constexpr HitType hitType() const noexcept { return m_HitType; }
  constexpr const SiPMHit* parent() const noexcept { return m_ParentPtr; }

  friend std::ostream& operator<<(std::ostream&, const SiPMHit&);
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

  // Always construct hits with values and no default constructor
  SiPMHit() = delete;

private:
  // Once a hit is constructed only
  // amplitude can be changed
  const double m_Time;
  const SiPMHit* m_ParentPtr;
  float m_Amplitude;
  const uint32_t m_Row;
  const uint32_t m_Col;
  const HitType m_HitType;
};

inline std::ostream& operator<<(std::ostream& out, const SiPMHit& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Hit <===\n";
  out << "Address: " << std::hex << std::addressof(obj) << "\n";
  out << "Hit time: " << std::dec << obj.m_Time << "\n";
  out << "Hit relative amplitude: " << obj.m_Amplitude << "\n";
  out << "Hit position on sensor: " << obj.m_Row << " - " << obj.m_Col << "\n";
  out << "Hit type: ";
  switch (obj.m_HitType) {
    case SiPMHit::HitType::kPhotoelectron:
      out << "Photoelectron\n";
      break;
    case SiPMHit::HitType::kDarkCount:
      out << "Dark count\n";
      break;
    case SiPMHit::HitType::kOpticalCrosstalk:
      out << "Optical crosstalk\n";
      break;
    case SiPMHit::HitType::kFastAfterPulse:
      out << "Afterpulse (fast)\n";
      break;
    case SiPMHit::HitType::kSlowAfterPulse:
      out << "Afterpulse (slow)\n";
      break;
    case SiPMHit::HitType::kDelayedOpticalCrosstalk:
      out << "Delayed optical crosstalk\n";
      break;
  }
  return out;
}
} // namespace sipm
#endif /* SIPM_SIPMHITS_H */
