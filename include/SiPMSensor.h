/** @class sipm::SiPMSensor SimSiPM/SimSiPM/SiPMSensor.h SiPMSensor.h
 *
 *  @brief Main class used to simulate a SiPM
 *
 *  This class provides all the methods to simulate a SiPM sensor.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMSENSOR_H
#define SIPM_SIPMSENSOR_H
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "SiPMAnalogSignal.h"
#include "SiPMDebugInfo.h"
#include "SiPMHit.h"
#include "SiPMProperties.h"
#include "SiPMRandom.h"
#include "SiPMTypes.h"

namespace sipm {
class SiPMSensor {
public:
  /// @brief SiPMSensor constructor from a @ref SiPMProperties instance
  /** Instantiates a SiPMSensor with parameter specified in the SiPMProperties.
   */
  SiPMSensor(const SiPMProperties&);

  SiPMSensor();

  /// @brief Returns the @ref SiPMProperties class stored in the SiPMSensor
  const SiPMProperties& properties() const { return m_Properties; }

  SiPMProperties& properties() { return m_Properties; }

  /// @brief Returns the @ref SiPMAnalogSignal stored in the SiPMSensor
  /** Used to get the generated signal from the sensor. This method should be
   * run after @ref runEvent otherwise it will return only electronic noise.
   */
  SiPMAnalogSignal signal() const { return m_Signal; }

  /// @brief Returns vector containing all SiPMHits
  /** This method allows to get all the hits generated in the simulation
   * process, including noise hits.
   */
  std::vector<SiPMHit> hits() const { return m_Hits; }

  /// @brief Returns vector containing history of hits
  /**
   * Returns a vector containing the index of the corresponding parent hit
   * for each hit. If the hit has no parent (e.g. DCR hit) the
   * index is set to -1.
   * This allows to get the complete chain of hits generation.
   */
  std::vector<int32_t> hitsGraph() const { return m_HitsGraph; }

  /// @brief Returns the @ref SiPMRandom rng used by SiPMSensor
  const SiPMRandom rng() const { return m_rng; }

  SiPMRandom& rng() { return m_rng; }

  /// @brief Returns a @ref SiPMDebugInfo struct with MC-Truth values
  SiPMDebugInfo debug() const {
    return SiPMDebugInfo{static_cast<uint32_t>(m_PhotonTimes.size()), m_nPe, m_nDcr, m_nXt, m_nDXt, m_nAp};
  }

  /// @brief Sets a property using its name
  /** For a list of available SiPM properties names @sa SiPMProperties.
   * This method uses a key/value to set the corresponding property.
   */
  void setProperty(const std::string&, const double);

  /// @brief Sets a different SiPMProperties for the SiPMSensor.
  /** Changes the underlying SiPMProperties object with a new one.
   */
  void setProperties(const SiPMProperties&);

  /// @brief Adds a single photon to the list of photons to be simulated
  void addPhoton(const double);

  /// @brief Adds a single photon to the list of photons to be simulated
  void addPhoton(const double, const double);

  /// @brief Adds multiple photons to the list of photons to be simulated at once
  void addPhotons(const std::vector<double>&);

  /// @brief Adds multiple photons to the list of photons to be simulated at once
  void addPhotons(const std::vector<double>&, const std::vector<double>&);

  /// @brief Runs a complete SiPM event
  void runEvent();

  /// @brief Resets internal state of the SiPMSensor
  /** Resets the SiPMSensor to a fresh state
   * so it can be used again for a new event. */
  void resetState();

  friend std::ostream& operator<<(std::ostream&, const SiPMSensor&);
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

private:
  double evaluatePde(const double) const;
  constexpr bool isInSensor(const int32_t r, const int32_t c) const noexcept {
    const int32_t nSideCells = m_Properties.nSideCells();
    return (r >= 0) & (c >= 0) & (r < nSideCells) & (c < nSideCells);
  }
  bool isValidHit(const SiPMHit&) const;
  pair<uint32_t> hitCell() const;
  std::vector<float> signalShape() const;

  void addDcrEvents();
  void addPhotoelectrons();
  void addCorrelatedNoise();

  SiPMHit generateXtHit(const SiPMHit&) const;
  SiPMHit generateApHit(const SiPMHit&) const;

  void calculateSignalAmplitudes();
  void generateSignal();

  SiPMProperties m_Properties;
  mutable SiPMRandom m_rng;

  uint32_t m_nTotalHits = 0;
  uint32_t m_nPe = 0;
  uint32_t m_nDcr = 0;
  uint32_t m_nXt = 0;
  uint32_t m_nDXt = 0;
  uint32_t m_nAp = 0;

  std::vector<double> m_PhotonTimes;
  std::vector<double> m_PhotonWavelengths;
  std::vector<SiPMHit> m_Hits;
  std::vector<int32_t> m_HitsGraph;

  std::vector<float> m_SignalShape;
  SiPMAnalogSignal m_Signal;
};

} // namespace sipm
#endif /* SIPM_SIPMSENSOR_H */
