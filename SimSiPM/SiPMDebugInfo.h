namespace sipm {
struct SiPMDebugInfo {
  SiPMDebugInfo(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) noexcept;
  const uint32_t nPhotons, nPhotoelectrons, nDcr, nXt, nAp;
};

SiPMDebugInfo::SiPMDebugInfo(uint32_t aPh, uint32_t aPe, uint32_t aDcr,
                             uint32_t aXt, uint32_t aAp) noexcept
  : nPhotons(aPh), nPhotoelectrons(aPe), nDcr(aDcr), nXt(aXt), nAp(aAp) {}

} /* namespace sipm */
