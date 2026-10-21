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
#include <cstdint>
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
  explicit SiPMSensor(const SiPMProperties&);

  SiPMSensor();

  /// @brief Returns the @ref SiPMProperties class stored in the SiPMSensor
  const SiPMProperties& properties() const { return m_Properties; }

  /// @brief Returns the @ref SiPMAnalogSignal of the last event
  /** If the last event was run with runEvent(false) the signal is generated
   * here, on first use, from the hits already simulated. Because of this
   * signal() may modify the sensor internals: do not call it concurrently
   * on the same sensor.
   */
  SiPMAnalogSignal signal() const {
    if (m_SignalPending) { buildSignal(); }
    return m_Signal;
  }

  /// @brief Returns the list of hits generated in the last event (by const reference)
  /** Elements are valid until the next call to resetState() or runEvent().
   * Copy individual hits if you need to keep them longer.
   */
  const std::vector<SiPMHit>& hits() const { return m_Hits; }

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

  /// @brief Override the computed signal shape with a user-supplied one.
  /** The shape vector is used verbatim (no gain scaling, no normalization).
   *  Rise/fall time flags are set to -1 in properties.  A subsequent call to
   *  setRiseTime / setFallTimeFast / setFallTimeSlow restores computed shapes.
   *  @param shape  Waveform samples (arbitrary units, user is responsible for normalisation)
   *  @param sampling  Sampling period in ns
   */
  void setSignalShape(const std::vector<float>& shape, double sampling);
  void setSignalShape(const std::vector<double>& shape, double sampling);

  /// @brief Adds a single photon to the list of photons to be simulated
  void addPhoton(const double);

  /// @brief Adds a single photon to the list of photons to be simulated
  void addPhoton(const double, const double);

  /// @brief Adds multiple photons to the list of photons to be simulated at once
  void addPhotons(const std::vector<double>&);

  /// @brief Adds multiple photons to the list of photons to be simulated at once
  void addPhotons(const std::vector<double>&, const std::vector<double>&);

  /// @brief Runs a SiPM event
  /** Simulates the hits (dark counts, photoelectrons, crosstalk, afterpulses
   * and cell recovery). With @p generateSignal = true (default) the analog
   * signal is generated too. With false only the hits are simulated, which
   * is much cheaper; the signal is then generated automatically the first
   * time it is requested (@ref signal), from the same hits. For a given seed
   * the hits and the signal are the same in both cases.
   * @param generateSignal Generate the analog signal now
   */
  void runEvent(const bool generateSignal = true);

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
  pair<uint32_t> hitUniform() const;
  pair<uint32_t> hitCircle() const;
  pair<uint32_t> hitGaussian(float x, float y) const;
  pair<uint32_t> hitCell(const std::vector<float>& gauss, uint32_t photonIdx) const;
  void signalShape();
  void updatePdeTable();

  void addDcrEvents();
  void addPhotoelectrons();
  void addCorrelatedNoise();

  SiPMHit generateXtHit(const SiPMHit&, int32_t parentIdx) const;
  SiPMHit generateApHit(const SiPMHit&, int32_t parentIdx) const;

  void calculateSignalAmplitudes();
  void buildSignal() const;
  void generateSignal() const;

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

  std::vector<float> m_SignalShape;
  float m_SignalNorm = 0; // gain / peak of the analytic shape, used by the recursive convolution
  std::vector<int32_t> m_CellHead; // per-cell first hit index, all -1 between events
  std::vector<double> m_PdeX, m_PdeSlope, m_PdeIntercept; // flat copy of pdeSpectrum segments
  // Mutable: the signal is generated lazily from the const signal() accessor
  mutable SiPMAnalogSignal m_Signal;
  mutable bool m_SignalPending = false; // hits simulated but signal not generated yet
};

} // namespace sipm
#endif /* SIPM_SIPMSENSOR_H */
