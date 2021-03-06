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
#include <stdint.h>
#ifndef SIPM_SIPMDEBUGINFO_H
#define SIPM_SIPMDEBUGINFO_H
namespace sipm {
struct SiPMDebugInfo {
  inline SiPMDebugInfo(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) noexcept;
  const uint32_t nPhotons;         ///< Number of photons given as input.
  const uint32_t nPhotoelectrons;  ///< Number of photoelectrons (hitted cells).
  const uint32_t nDcr;             ///< Number of DCR events generated.
  const uint32_t nXt;              ///< Number of XT events generated.
  const uint32_t nAp;              ///< Number of AP events generated.
};

/** @brief Constructor of SiPMDebugInfo */
inline SiPMDebugInfo::SiPMDebugInfo(uint32_t aPh, uint32_t aPe, uint32_t aDcr, uint32_t aXt, uint32_t aAp) noexcept
    : nPhotons(aPh), nPhotoelectrons(aPe), nDcr(aDcr), nXt(aXt), nAp(aAp) {}

} /* namespace sipm */
#endif /* SIPM_SIPMDEBUGINFO_H */
