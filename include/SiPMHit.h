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

#include <algorithm>
#include <ostream>
#include <stdint.h>
#include <memory>
#include <vector>

namespace sipm {

class SiPMHit {
public:
  /** @enum HitType
   * Used to distinguish between hits generated by different processes
   */
  enum class HitType {
    kPhotoelectron,    ///< Hit generated by a photoelectron
    kDarkCount,        ///< Hit generated by a dark count
    kOpticalCrosstalk, ///< Hit generated by an optical crosstalk
    kDelayedOpticalCrosstalk, ///< Hit generated by a delayed optical crosstalk
    kFastAfterPulse,       ///< Hit generated by a fast afterpulse
    kSlowAfterPulse        ///< Hit generated by a slow afterpulse
  };

  SiPMHit() = default;
  /** @brief Constructor of SiPMHit */
  SiPMHit(const double time, const double amp, const uint32_t r, const uint32_t c, const HitType type) noexcept
    : m_Time(time), m_Amplitude(amp), m_Row(r), m_Col(c), m_HitType(type) {}

  /** @brief Constructor of SiPMHit */
  SiPMHit(const double time, const double amp, const uint32_t r, const uint32_t c, const HitType type,const SiPMHit& aHit) noexcept
    : m_Time(time), m_Amplitude(amp), m_Row(r), m_Col(c), m_HitType(type), m_ParentHit(std::make_shared<SiPMHit>(aHit)) {}


  /// @brief Operator used to sort hits
  /**
   * Hits are sorted based on theyr time parameter:
   * @f[Hit_1 < Hit_2 \Leftrightarrow Hit_1.time < Hit_2.time @f]
   */
  inline bool operator<(const SiPMHit& rhs) const noexcept { return m_Time < rhs.m_Time; }
  inline bool operator<=(const SiPMHit& rhs) const noexcept { return m_Time <= rhs.m_Time; }

  /// @brief Operator used to check if the hit is in the same cell
  /**
   * Hits are considered equal if they have same row and column
   */
  inline bool operator==(const SiPMHit& rhs) const noexcept { return ((m_Row == rhs.m_Row) && (m_Col == rhs.m_Col)); }
  inline bool operator!=(const SiPMHit& rhs) const noexcept { return ((m_Row != rhs.m_Row) || (m_Col != rhs.m_Col)); }

  /// @brief Returns hit time
  double time() const { return m_Time; }
  /// @brief Returns row of hitted cell
  uint32_t row() const { return m_Row; }
  /// @brief Returns column of hitted cell
  uint32_t col() const { return m_Col; }
  /// @brief Returns amplitude of the signal produced by the hit
  double amplitude() const { return m_Amplitude; }
  /// @brief Used to modify the amplitude if needed
  double& amplitude() { return m_Amplitude; }
  /// @brief Returns hit type to identify the hits
  HitType hitType() const { return m_HitType; }
  /// @brief Adds a hit to the list of childrens of this hit
  void addChildren(const SiPMHit& aHit) {m_ChildrenHits.push_back(std::make_shared<SiPMHit>(aHit));}
  /// @brief Returns pointer to parent hit (nullptr for dark counts and photoelectrons)
  std::shared_ptr<SiPMHit> parent() const { return m_ParentHit; }
  /// @brief Returns vector of pointers to children hits
  std::vector<std::shared_ptr<SiPMHit>> childrens() const {return m_ChildrenHits; }

  friend std::ostream& operator<<(std::ostream&, SiPMHit const&);

private:
  double m_Time;
  double m_Amplitude;
  uint32_t m_Row;
  uint32_t m_Col;
  HitType m_HitType;
  std::shared_ptr<SiPMHit> m_ParentHit = nullptr;
  std::vector<std::shared_ptr<SiPMHit>> m_ChildrenHits;
};

inline std::ostream& operator<<(std::ostream& os, const SiPMHit& x) {
  os << "=== SiPM Hit Start ===\n";
  os << "Time of hit: " << x.m_Time << "\n";
  os << "Hit relative amplitude: " << x.m_Amplitude << "\n";
  os << "Hit position on sensor: " << x.m_Row << " - " << x.m_Col << "\n";
  os << "Hit type: ";
  switch (x.m_HitType) {
    case SiPMHit::HitType::kPhotoelectron:
      os << "Photoelectron\n";
      break;
    case SiPMHit::HitType::kDarkCount:
      os << "Dark count\n";
      break;
    case SiPMHit::HitType::kOpticalCrosstalk:
      os << "Optical crosstalk\n";
      break;
    case SiPMHit::HitType::kFastAfterPulse:
      os << "Afterpulse (fast)\n";
      break;
    case SiPMHit::HitType::kSlowAfterPulse:
      os << "Afterpulse (slow)\n";
      break;
    case SiPMHit::HitType::kDelayedOpticalCrosstalk:
      os << "Delayed optical crosstalk\n";
      break;
  }
  os << "=== SiPM Hit End ===";
  return os;
}

} // namespace sipm
#endif /* SIPM_SIPMHITS_H */
