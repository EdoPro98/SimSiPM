/** @struct sipm::SiPMDebugInfo SimSiPM/SimSiPM/SiPMDebugInfo.h SiPMDebugInfo.h
 *
 *  @brief Stores MC-Truth informations.
 *
 *  This class is used to store some MC-Truth informations about the generated
 *  event for debug purposes.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMDEBUGINFO_H
#define SIPM_SIPMDEBUGINFO_H

#include <ostream>
#include <stdint.h>

namespace sipm {

struct SiPMDebugInfo {
  SiPMDebugInfo() = default;
  inline SiPMDebugInfo(uint32_t, uint32_t, uint32_t, uint32_t,uint32_t, uint32_t) noexcept;
  uint32_t nPhotons;        ///< Number of photons given as input.
  uint32_t nPhotoelectrons; ///< Number of photoelectrons (hitted cells).
  uint32_t nDcr;            ///< Number of DCR events generated.
  uint32_t nXt;             ///< Number of XT events generated.
  uint32_t nDXt;            ///< Number of delayed XT events generated.
  uint32_t nAp;             ///< Number of AP events generated.
};

/** @brief Constructor of SiPMDebugInfo */
inline SiPMDebugInfo::SiPMDebugInfo(uint32_t aPh, uint32_t aPe, uint32_t aDcr, uint32_t aXt, uint32_t aDXt, uint32_t aAp) noexcept
  : nPhotons(aPh), nPhotoelectrons(aPe), nDcr(aDcr), nXt(aXt), nDXt(aDXt), nAp(aAp) {}

inline std::ostream& operator<<(std::ostream& os, const SiPMDebugInfo& x) {
  os << "===> SiPM Debug Info Start <===\n";
  os << "Number of photons arrived to the sensor: " << x.nPhotons << "\n";
  os << "Number of photons detected (photoelectrons): " << x.nPhotoelectrons << "\n";
  os << "Number of dark count events (DCR): " << x.nDcr << "\n";
  os << "Number of optical crosstalk events (XT): " << x.nXt << "\n";
  os << "Number of delayed optical crosstalk events (DXT): " << x.nDXt << "\n";
  os << "Number of afterpulsing events (AP): " << x.nAp << "\n";
  os << "===> SiPM Debug Info End <===";
  return os;
}
} /* namespace sipm */
#endif /* SIPM_SIPMDEBUGINFO_H */
