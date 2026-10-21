/** @class sipm::SiPMSensor SiPMSensor.h
 *
 *  @brief Main class used to simulate a SiPM
 *
 *  This class provides all the methods to simulate a SiPM sensor. Typical use:
 *  add the photons with addPhoton()/addPhotons(), call runEvent() and read the
 *  result with hits(), signal() and debug(); call resetState() before the next
 *  event. runEvent(false) simulates only the hits and leaves the signal to be
 *  generated on demand.
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
  /// @brief SiPMSensor constructor from a @ref SiPMProperties instance
  /// Instantiates a SiPMSensor with parameter specified in the SiPMProperties.
  /// @param properties Properties of the sensor
  explicit SiPMSensor(const SiPMProperties& properties);

  /// @brief SiPMSensor constructor with default @ref SiPMProperties
  SiPMSensor();

  /// @brief The @ref SiPMProperties of the SiPMSensor
  /// @return Const reference to the properties
  const SiPMProperties& properties() const { return m_Properties; }

  /// @brief Returns the @ref SiPMAnalogSignal of the last event
  /// @brief The @ref SiPMAnalogSignal of the last event
  /// If the last event was run with runEvent(false) the signal is generated here, on first use, from the hits already simulated.
  /// Because of this signal() may modify the sensor internals: do not call it concurrently on the same sensor.
  /// Before any event it is empty.
  /// @return Copy of the signal
  SiPMAnalogSignal signal() const {
    if (m_SignalPending) { buildSignal(); }
    return m_Signal;
  }

  /// @brief Returns the list of hits generated in the last event (by const reference)
  /// @brief The hits generated in the last event
  /// Elements are valid until the next call to resetState() or runEvent(). Copy individual hits if you need to keep them longer.
  /// @return Const reference to the list of hits
  const std::vector<SiPMHit>& hits() const { return m_Hits; }

  /// @brief The @ref SiPMRandom rng used by SiPMSensor
  /// @return Copy of the random number generator
  const SiPMRandom rng() const { return m_rng; }

  /// @brief The @ref SiPMRandom rng used by SiPMSensor, for seeding
  /// @return Reference to the random number generator
  SiPMRandom& rng() { return m_rng; }

  /// @brief MC-Truth counters of the last event
  /// @return A @ref SiPMDebugInfo with the counters
  SiPMDebugInfo debug() const {
    return SiPMDebugInfo{static_cast<uint32_t>(m_PhotonTimes.size()), m_nPe, m_nDcr, m_nXt, m_nDXt, m_nAp};
  }

  /// @brief Sets a property using its name
  /// @brief Sets a property using its name
  /// For a list of available SiPM properties names @sa SiPMProperties::setProperty. The signal shape is recomputed.
  /// @param prop Name of the property
  /// @param val Value of the property
  void setProperty(const std::string& prop, const double val);

  /// @brief Sets a different SiPMProperties for the SiPMSensor.
  /// @brief Sets a different SiPMProperties for the SiPMSensor
  /// Changes the underlying SiPMProperties object with a new one. The signal shape is recomputed.
  /// @param properties The new properties
  void setProperties(const SiPMProperties& properties);

  /// @brief Override the computed signal shape with a user-supplied one.
  /** The shape vector is used verbatim (no gain scaling, no normalization).
   *  Rise/fall time flags are set to -1 in properties.  A subsequent call to
   *  setRiseTime / setFallTimeFast / setFallTimeSlow restores computed shapes.
   *  @param shape  Waveform samples (arbitrary units, user is responsible for normalisation)
   *  @param sampling  Sampling period in ns
   */
  void setSignalShape(const std::vector<float>& shape, double sampling);
  /// @overload
  /// Same as above, for a double precision shape (stored as float).
  /// @param shape Waveform samples
  /// @param sampling Sampling period in ns
  void setSignalShape(const std::vector<double>& shape, double sampling);

  /// @brief Adds a single photon to the list of photons to be simulated
  /// @param time Arrival time of the photon in ns, relative to the start of the signal
  void addPhoton(const double time);

  /// @brief Adds a single photon with its wavelength (needed for PdeType::kSpectrumPde)
  /// @param time Arrival time of the photon in ns
  /// @param wavelength Wavelength of the photon, same unit as the PDE spectrum
  void addPhoton(const double time, const double wavelength);

  /// @brief Adds multiple photons to the list of photons to be simulated at once
  /// @param times Arrival times of the photons in ns
  void addPhotons(const std::vector<double>& times);

  /// @brief Adds multiple photons with their wavelengths at once
  /// If the sizes differ the extra entries are ignored and a message is printed on std::cerr.
  /// @param times Arrival times of the photons in ns
  /// @param wavelengths Wavelengths of the photons, same unit as the PDE spectrum
  void addPhotons(const std::vector<double>& times, const std::vector<double>& wavelengths);

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

  /// @brief Prints the properties and the debug info of the sensor
  friend std::ostream& operator<<(std::ostream&, const SiPMSensor&);
  /// @brief Description of the sensor
  /// @return String printed by operator<<
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

private:
  /// @brief PDE of a photon of a given wavelength, linearly interpolated from the spectrum
  /// @param x Wavelength of the photon
  /// @return PDE in [0, 1], 0 if no spectrum is set
  double evaluatePde(const double x) const;
  /// @brief Checks that a cell is inside the sensor
  /// @param r Row of the cell
  /// @param c Column of the cell
  /// @return true if the cell exists
  constexpr bool isInSensor(const int32_t r, const int32_t c) const noexcept {
    const int32_t nSideCells = m_Properties.nSideCells();
    return (r >= 0) & (c >= 0) & (r < nSideCells) & (c < nSideCells);
  }
  /// @brief Cell hit with a uniform distribution
  /// @return Row and column of the cell
  pair<uint32_t> hitUniform() const;
  /// @brief Cell hit with the kCircle distribution
  /// @return Row and column of the cell
  pair<uint32_t> hitCircle() const;
  /// @brief Cell hit with the kGaussian distribution, from two standard normal numbers
  /// Points outside +-1.64 sigma on any axis are replaced by a uniform hit.
  /// @param x Standard normal number for the row
  /// @param y Standard normal number for the column
  /// @return Row and column of the cell
  pair<uint32_t> hitGaussian(float x, float y) const;
  /// @brief Cell hit by a photon with the distribution set in the properties
  /// @param gauss Standard normal (x, y) pairs, one per photon, used only by kGaussian
  /// @param photonIdx Index of the photon
  /// @return Row and column of the cell
  pair<uint32_t> hitCell(const std::vector<float>& gauss, uint32_t photonIdx) const;
  /// @brief Computes the pulse shape of one photoelectron into m_SignalShape
  /// Sum of a fast and an optional slow decaying exponential minus a rising one, scaled to a peak equal to the gain. Does nothing for a custom shape.
  /// Must be called every time the properties change.
  void signalShape();
  /// @brief Rebuilds the flat PDE lookup table from the spectrum in the properties
  /// Must be called every time the properties change.
  void updatePdeTable();

  /// @brief Adds the dark counts generated in the signal window (exponential inter-arrival times)
  void addDcrEvents();
  /// @brief Turns the photons into hits, applying the PDE and the hit distribution
  void addPhotoelectrons();
  /// @brief Adds crosstalk and afterpulse hits generated by the existing hits (also by the ones just added)
  void addCorrelatedNoise();

  /// @brief Generates a crosstalk hit in a neighbour cell of a hit
  /// @param xtGen Hit that generates the crosstalk
  /// @param parentIdx Index of @p xtGen in the hit list
  /// @return The new hit, delayed with probability dxt()
  SiPMHit generateXtHit(const SiPMHit& xtGen, int32_t parentIdx) const;
  /// @brief Generates an afterpulse in the same cell of a hit, with a fast or slow exponential delay
  /// @param apGen Hit that generates the afterpulse
  /// @param parentIdx Index of @p apGen in the hit list
  /// @return The new hit
  SiPMHit generateApHit(const SiPMHit& apGen, int32_t parentIdx) const;

  /// @brief Applies cell-to-cell gain variation and cell recovery to the amplitudes of the hits
  /// A hit in a cell that was already hit has its amplitude reduced by 1 - exp(-delay / recoveryTime). May reorder the hit list (parent indexes are remapped).
  void calculateSignalAmplitudes();
  /// @brief Builds the signal of the current hits: electronic noise baseline plus the pulses of all hits
  /// Clears the pending-signal flag.
  void buildSignal() const;
  /// @brief Adds the pulses of all hits to the signal
  /// Direct sum of shifted pulses, or a recursive convolution when there are many hits and the pulse is analytic. Hits outside the signal window are skipped.
  void generateSignal() const;

  SiPMProperties m_Properties;  ///< Properties of the sensor
  mutable SiPMRandom m_rng;  ///< Random number generator (mutable: the signal is generated from const methods)

  uint32_t m_nTotalHits = 0;  ///< Number of hits of the event
  uint32_t m_nPe = 0;  ///< Number of hit cells (all hit types)
  uint32_t m_nDcr = 0;  ///< Number of dark counts
  uint32_t m_nXt = 0;  ///< Number of crosstalk hits (delayed included)
  uint32_t m_nDXt = 0;  ///< Number of delayed crosstalk hits
  uint32_t m_nAp = 0;  ///< Number of afterpulses

  std::vector<double> m_PhotonTimes;  ///< Arrival times of the photons in ns
  std::vector<double> m_PhotonWavelengths;  ///< Wavelengths of the photons
  std::vector<SiPMHit> m_Hits;  ///< Hits of the event

  std::vector<float> m_SignalShape;  ///< Pulse shape of one photoelectron
  float m_SignalNorm = 0;  ///< gain / peak of the analytic shape, used by the recursive convolution
  std::vector<int32_t> m_CellHead;  ///< Per-cell first hit index used to group hits by cell, all -1 between events
  std::vector<double> m_PdeX;         ///< Start wavelength of each PDE segment, plus the last wavelength
  std::vector<double> m_PdeSlope;     ///< Slope of each PDE segment
  std::vector<double> m_PdeIntercept; ///< Intercept of each PDE segment
  mutable SiPMAnalogSignal m_Signal;  ///< Signal of the event, mutable because it is generated lazily from the const signal()
  mutable bool m_SignalPending = false; ///< Hits simulated but signal not generated yet
};

} // namespace sipm
#endif /* SIPM_SIPMSENSOR_H */
