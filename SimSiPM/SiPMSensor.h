#include "SiPMProperties.h"

#include <algorithm>
#include <array>
#include <unordered_set>

#include "SiPMAnalogSignal.h"
#include "SiPMDebugInfo.h"
#include "SiPMHit.h"
#include "SiPMRandom.h"

#ifndef SIPM_SIPMSENSOR_H
#define SIPM_SIPMSENSOR_H

namespace sipm {

class SiPMSensor {
public:
  enum class PrecisionLevel { kFull, kFast };

  SiPMSensor(const SiPMProperties&) noexcept;
  SiPMSensor() = default;

  void dumpSettings() const;

  const SiPMProperties& properties() const { return m_Properties; }
  const SiPMAnalogSignal& signal() const { return m_Signal; }
  const SiPMRandom& rng() const { return m_rng; }
  SiPMProperties& properties() { return m_Properties; }
  SiPMRandom& rng() { return m_rng; }
  SiPMDebugInfo debug() {
    return SiPMDebugInfo(m_PhotonTimes.size(), m_nPe, m_nDcr, m_nXt, m_nAp);
  }

  void addPhotons(const std::vector<double>&, const std::vector<double>&);
  inline void addPhotons(const std::vector<double>& aTimes) {
    resetState();
    m_PhotonTimes = aTimes;
  }
  void addPhotons() { resetState(); }
  void runEvent();
  void setProperty(const std::string&, const double);
  void setProperties(const std::vector<std::string>&,
                     const std::vector<double>&);
  void setPrecisionLevel(const PrecisionLevel x) { m_PrecisionLevel = x; }

private:
  const double evaluatePde(const double) const;
  const bool isDetected(const double aPde) { return m_rng.Rand() < aPde; }
  const bool isInSensor(const int32_t, const int32_t) const;
  const std::pair<int32_t, int32_t> hitCell() const;
  const std::pair<std::vector<uint32_t>, std::unordered_set<uint32_t>>
  getUniqueId() const;
  const std::vector<double> signalShape() const;
  void sortHits() { std::sort(m_Hits.begin(), m_Hits.end()); }

  void addDcrEvents();
  void addPhotoelectrons();
  void addXtEvents();
  void addApEvents();
  void calculateSignalAmplitudes();
  void generateSignal() __attribute__((hot, optimize("Ofast", "fast-math")));
  void resetState();

  SiPMProperties m_Properties;
  mutable SiPMRandom m_rng;

  uint32_t m_nTotalHits = 0;
  uint32_t m_nPe = 0;
  uint32_t m_nDcr = 0;
  uint32_t m_nXt = 0;
  uint32_t m_nAp = 0;

  std::vector<double> m_PhotonTimes;
  std::vector<double> m_PhotonWavelengths;
  std::vector<SiPMHit> m_Hits;

  std::vector<double> m_SignalShape;
  SiPMAnalogSignal m_Signal;

  PrecisionLevel m_PrecisionLevel = PrecisionLevel::kFull;
};

} // namespace sipm
#endif /* SIPM_SIPMSENSOR_H */
