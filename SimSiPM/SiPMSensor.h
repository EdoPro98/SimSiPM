#include "SiPMProperties.h"

#include <algorithm>
#include <unordered_set>
#include <array>

#include "SiPMAnalogSignal.h"
#include "SiPMHit.h"
#include "SiPMRandom.h"
#include "SiPMRandomvect.h"

#ifndef SIPM_SIPMSENSOR_H
#define SIPM_SIPMSENSOR_H

namespace sipm{

class SiPMSensor{
public:

  enum class PrecisionLevel{kFull, kFast};

  // Special
  SiPMSensor(const SiPMProperties&);
  SiPMSensor() = default;

  // Debugging
  void dumpSettings()const;
  void setVerbose();
  void setPrecisionLevel();

  // Getters
  const SiPMProperties& properties()const{return m_Properties;}
  const SiPMAnalogSignal& signal()const{return m_Signal;}

  // Setters
  void addPhotons(const std::vector<double>&, const std::vector<double>&);
  void addPhotons(const std::vector<double>& aTimes){resetState();m_PhotonTimes = aTimes;}
  void addPhotons(){resetState();}
  void runEvent();
  void setProperty(const std::string&, const double);
  void setProperties(const std::vector<std::string>&, const std::vector<double>&);


private:
  // Helper functions
  const double evaluatePde(const double)const;
  static const bool isDetected(const double aPde){return Rand()<aPde;}
  const bool isInSensor(const int32_t, const int32_t)const;
  const std::array<int32_t,2> hitCell()const;
  const std::pair<std::vector<uint32_t>, std::unordered_set<uint32_t>>
   getUniqueId()const;
  const std::vector<double> signalShape()const;
  inline void sortHits(){std::sort(m_Hits.begin(), m_Hits.end());}

  // SiPM functions
  void addDcrEvents();
  void addPhotoelectrons();
  void addXtEvents();
  void addApEvents();
  void calculateSignalAmplitudes();
  void generateSignal();
  void resetState();

  SiPMProperties m_Properties;

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

  uint32_t m_verbsityLevel = 0;

};

} /* NAMESPACE_SIPM */
#endif /* SIPM_SIPMSENSOR_H */
