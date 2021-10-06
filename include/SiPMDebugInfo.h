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

#include <iomanip>
#include <iostream>
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
  friend std::ostream& operator<< (std::ostream&, const SiPMDebugInfo&);
};

/** @brief Constructor of SiPMDebugInfo */
inline SiPMDebugInfo::SiPMDebugInfo(uint32_t aPh, uint32_t aPe, uint32_t aDcr, uint32_t aXt, uint32_t aDXt, uint32_t aAp) noexcept
  : nPhotons(aPh), nPhotoelectrons(aPe), nDcr(aDcr), nXt(aXt), nDXt(aDXt), nAp(aAp) {}

inline std::ostream& operator<< (std::ostream& out, const SiPMDebugInfo& obj){
  out << std::setprecision(2)<<std::fixed;
  out << "Address :"<<std::addressof(obj)<<"\n";
  out << "===> SiPM Debug Info <===\n";
  out << "Number of photons arrived to the sensor: " << obj.nPhotons << "\n";
  out << "Number of photons detected (photoelectrons): " << obj.nPhotoelectrons << "\n";
  out << "Number of dark count events (DCR): " << obj.nDcr << "\n";
  out << "Number of optical crosstalk events (XT): " << obj.nXt << "\n";
  out << "Number of delayed optical crosstalk events (DXT): " << obj.nDXt << "\n";
  out << "Number of afterpulsing events (AP): " << obj.nAp << "\n";
  return out;
}
} /* namespace sipm */
#endif /* SIPM_SIPMDEBUGINFO_H */
