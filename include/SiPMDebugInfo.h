/** @struct sipm::SiPMDebugInfo SiPMDebugInfo.h
 *
 *  @brief Stores MC-Truth information.
 *
 *  This class is used to store some MC-Truth information about the generated
 *  event for debug purposes.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMDEBUGINFO_H
#define SIPM_SIPMDEBUGINFO_H

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace sipm {
struct SiPMDebugInfo {
  /// @brief Constructor of SiPMDebugInfo
  /// @param aPh Number of photons impinging on the sensor
  /// @param aPe Number of hit cells
  /// @param aDcr Number of dark count events
  /// @param aXt Number of optical crosstalk events (delayed included)
  /// @param aDXt Number of delayed optical crosstalk events
  /// @param aAp Number of afterpulses
  constexpr SiPMDebugInfo(const uint32_t aPh, const uint32_t aPe, const uint32_t aDcr, const uint32_t aXt,
                          const uint32_t aDXt, const uint32_t aAp) noexcept
    : nPhotons(aPh), nPhotoelectrons(aPe), nDcr(aDcr), nXt(aXt), nDXt(aDXt), nAp(aAp) {}

  const uint32_t nPhotons;        ///< Number of photons impinging on the sensor surface
  const uint32_t nPhotoelectrons; ///< Number of hit cells: photoelectrons, dark counts, crosstalk and afterpulses
  const uint32_t nDcr;            ///< Number of DCR events generated
  const uint32_t nXt;             ///< Number of XT events generated: XT and DXT
  const uint32_t nDXt;            ///< Number of DXT events generated
  const uint32_t nAp;             ///< Number of AP events generated
  /// @brief Prints all the counters
  friend std::ostream& operator<<(std::ostream&, const SiPMDebugInfo&);
  /// @brief Description of the counters
  /// @return String printed by operator<<
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }
};

inline std::ostream& operator<<(std::ostream& out, const SiPMDebugInfo& obj) {
  // The caller's stream format is restored before returning
  const std::ios_base::fmtflags flags = out.flags();
  const std::streamsize precision = out.precision();
  out << std::setprecision(2) << std::fixed;
  out << "Address :" << std::hex << std::addressof(obj) << "\n";
  out << "===> SiPM Debug Info <===\n";
  out << "Number of photons impinging to the sensor: " << std::dec << obj.nPhotons << "\n";
  out << "Number of photoelectrons detected: " << obj.nPhotoelectrons << "\n";
  out << "Number of dark count events (DCR): " << obj.nDcr << "\n";
  out << "Number of optical crosstalk events (XT + DTX): " << obj.nXt << "\n";
  out << "Number of delayed optical crosstalk events (DXT): " << obj.nDXt << "\n";
  out << "Number of afterpulsing events (AP): " << obj.nAp << "\n";
  out.flags(flags);
  out.precision(precision);
  return out;
}
} /* namespace sipm */
#endif /* SIPM_SIPMDEBUGINFO_H */
