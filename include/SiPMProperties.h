/** @class sipm::SiPMProperties SiPMProperties.h
 *
 *  @brief Class storing all the parameters that describe a SiPM.
 *
 *  This class stores all the parameters and values used to describe a SiPM
 *  sensor or signal. It also allows to switch on or off some noise effects
 *  and can set different levels of detail in the evaluation of PDE.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

#ifndef SIPM_SIPMPROPERTIES_H
#define SIPM_SIPMPROPERTIES_H

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace sipm {
class SiPMProperties {
public:
  /** @enum PdeType
   * @brief Used to set different methods to evaluate PDE for each photon.
   */
  enum class PdeType {
    kNoPde,      ///< No PDE applied, all photons will turn in photoelectrons
    kSimplePde,  ///< Same PDE value used for all photons
    kSpectrumPde ///< PDE calculated considering the wavelength of each photon
  };
  /** @enum HitDistribution
   * Used to describe how photoelectrons are distributed on the SiPM surface
   */
  enum class HitDistribution {
    kUniform, ///< Photons uniformly distributed on the sensor surface
    kCircle,  ///< 90% of photons uniform inside the circle inscribed in the sensor, the rest uniform in the corners
    kGaussian ///< Photons inside +-1.64 sigma (sigma = side/3.28) on both axes are gaussian distributed, the others uniform
  };

  /// @brief Creates a SiPM with the default properties
  SiPMProperties();

  /// @brief Reads the settings from a legacy key=value file
  /// One @c name=value pair per line, names as accepted by setProperty(); spaces are ignored and lines starting with # or / are comments.
  /// @param fname Path of the file
  /// @return Properties read (defaults for unset values, or all defaults if the file cannot be opened)
  static SiPMProperties readSettings(const std::string& fname);

  /// @brief Reads the settings from a JSON file
  /// Recognised keys: size, pitch, sampling, signalLength, risingTime, fallingTimeFast, fallingTimeSlow, slowComponentFraction,
  /// recoveryTime, snr, dcr, xt, dxt, dxtTau, ap, tauApFast, tauApSlow, apSlowFraction, ccgv, pde, the flags hasDcr, hasXt, hasDXt, hasAp,
  /// hasSlowComponent, pdeType and hitDistribution (enumerator name or integer) and pdeSpectrum ({"wavelengths": [...], "pde": [...]}).
  /// @param fname Path of the file
  /// @return Properties read (defaults for unset values, or all defaults if the file cannot be read or parsed)
  static SiPMProperties readSettingsJson(const std::string& fname);

  /// @brief Size of the sensor
  /// @return Side of the sensitive area in mm
  constexpr uint32_t size() const { return m_Size; }

  /// @brief Pitch of the cells
  /// @return Side of one cell in um
  constexpr uint32_t pitch() const { return m_Pitch; }

  /// @brief Total number of cells in the sensor
  /// @return Number of cells (nSideCells() squared)
  constexpr uint32_t nCells() const { return m_Ncells; }

  /// @brief Number of cells along one side of the sensor
  /// @return Cells per side
  constexpr uint32_t nSideCells() const { return m_SideCells; }

  /// @brief Number of points of the signal
  /// @return Number of samples (signal length / sampling)
  constexpr uint32_t nSignalPoints() const { return m_SignalPoints; }

  /// @brief Distribution of the hits on the sensor surface
  /// @return The @ref HitDistribution in use
  constexpr HitDistribution hitDistribution() const { return m_HitDistribution; }

  /// @brief Length of the simulated signal
  /// @return Signal length in ns
  constexpr double signalLength() const { return m_SignalLength; }

  /// @brief Sampling time of the signal
  /// @return Sampling time in ns
  constexpr double sampling() const { return m_Sampling; }

  /// @brief Rising time constant of the signal pulse @sa SiPMSensor::signalShape
  /// @return Time constant in ns
  constexpr double risingTime() const { return m_RiseTime; }

  /// @brief Falling time constant of the fast component of the pulse @sa SiPMSensor::signalShape
  /// @return Time constant in ns
  constexpr double fallingTimeFast() const { return m_FallTimeFast; }

  /// @brief Falling time constant of the slow component of the pulse @sa SiPMSensor::signalShape
  /// @return Time constant in ns
  constexpr double fallingTimeSlow() const { return m_FallTimeSlow; }

  /// @brief Weight of the slow component of the pulse @sa SiPMSensor::signalShape
  /// @return Fraction in [0, 1]
  constexpr double slowComponentFraction() const { return m_SlowComponentFraction; }

  /// @brief Recovery time constant of the cells
  /// @return Time constant in ns
  constexpr double recoveryTime() const { return m_RecoveryTime; }

  /// @brief Dark count rate
  /// @return Rate in Hz
  constexpr double dcr() const { return m_Dcr; }

  /// @brief Optical crosstalk probability
  /// @return Probability in [0, 1]
  constexpr double xt() const { return m_Xt; }

  /// @brief Fraction of the optical crosstalk that is delayed
  /// @return Fraction in [0, 1]
  constexpr double dxt() const { return m_DXt; }

  /// @brief Time constant of the delay of delayed optical crosstalk
  /// @return Time constant in ns
  constexpr double dxtTau() const { return m_DXtTau; }

  /// @brief Afterpulse probability
  /// @return Probability in [0, 1]
  constexpr double ap() const { return m_Ap; }

  /// @brief Time constant of the delay of fast afterpulses
  /// @return Time constant in ns
  constexpr double tauApFast() const { return m_TauApFastComponent; }

  /// @brief Time constant of the delay of slow afterpulses
  /// @return Time constant in ns
  constexpr double tauApSlow() const { return m_TauApSlowComponent; }

  /// @brief Fraction of the afterpulses that are slow
  /// @return Fraction in [0, 1]
  constexpr double apSlowFraction() const { return m_ApSlowFraction; }

  /// @brief Cell-to-cell gain variation
  /// @return Relative sigma of the gaussian spread of the cell amplitudes (0.05 = 5%)
  constexpr float ccgv() const { return m_Ccgv; }

  /// @brief Gain of the sensor
  /// @return Amplitude of the pulse of one photoelectron (peak of the signal shape)
  constexpr double gain() const { return m_Gain; }

  /// @brief Signal to noise ratio
  /// @return SNR in dB
  constexpr double snrdB() const { return m_SnrdB; }

  /// @brief Electronic noise
  /// @return Standard deviation of the gaussian baseline noise, in units of the one photoelectron amplitude (10^(-SNR/20))
  constexpr double snrLinear() const { return m_SnrLinear; }

  /// @brief Photon detection efficiency used with PdeType::kSimplePde
  /// @return PDE in [0, 1]
  constexpr double pde() const { return m_Pde; }

  /// @brief Photon detection efficiency versus wavelength, used with PdeType::kSpectrumPde
  /// @return Map wavelength -> PDE, sampled on a uniform grid of 32 points
  const std::map<double, double>& pdeSpectrum() const { return m_PdeSpectrum; }

  /// @brief Type of PDE calculation used
  /// @return The @ref PdeType in use
  constexpr PdeType pdeType() const { return m_HasPde; }

  /// @brief Dark counts simulation
  /// @return true if dark counts are simulated
  constexpr bool hasDcr() const { return m_HasDcr; }

  /// @brief Optical crosstalk simulation
  /// @return true if optical crosstalk is simulated
  constexpr bool hasXt() const { return m_HasXt; }

  /// @brief Delayed optical crosstalk simulation
  /// @return true if delayed optical crosstalk is simulated
  constexpr bool hasDXt() const { return m_HasDXt; }

  /// @brief Afterpulses simulation
  /// @return true if afterpulses are simulated
  constexpr bool hasAp() const { return m_HasAp; }

  /// @brief Slow component of the pulse @sa SiPMSensor::signalShape
  /// @return true if the slow component is used
  constexpr bool hasSlowComponent() const { return m_HasSlowComponent; }

  /// @brief User-supplied signal shape
  /// @return true if a shape set with SiPMSensor::setSignalShape is active
  constexpr bool hasCustomSignalShape() const { return m_HasCustomSignalShape; }

  /// @brief Registers a user-supplied signal shape
  /// Only sampling and number of points are stored here, the vector itself is stored in SiPMSensor. Sets rise/fall times to -1.
  /// @param sampling Sampling time of the shape in ns
  /// @param nPoints Number of samples of the shape
  void setCustomSignalShape(const double sampling, const uint32_t nPoints) {
    m_Sampling = sampling;
    m_SignalPoints = nPoints;
    m_SignalLength = static_cast<double>(nPoints) * sampling;
    m_RiseTime = -1;
    m_FallTimeFast = -1;
    m_FallTimeSlow = -1;
    m_HasCustomSignalShape = true;
  }

  /// @brief Sets a property using its name
  /// Names are case-insensitive: size, pitch, sampling, signallength, risetime, falltimefast, falltimeslow, slowcomponentfraction,
  /// recoverytime (or cellrecovery), tauapfast, tauapslow, ccgv, snr, pde, dcr, xt, dxt, ap. Setting pde, dcr, xt, dxt or ap also enables
  /// that effect. An unknown name prints a message on std::cerr and is ignored.
  /// @param prop Name of the property
  /// @param val Value of the property, in the units of the corresponding setter
  void setProperty(const std::string& prop, const double val);

  /// @brief Set size of SiPM sensitive area (side in mm)
  ///@param x Size of sipm sensor in mm
  constexpr void setSize(const double x) {
    m_Size = x;
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }

  /// @brief Set pitch of SiPM cells (side in um)
  /// @param x Size of sipm cell in um
  constexpr void setPitch(const double x) {
    m_Pitch = x;
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }

  /// @brief Set sampling time of the signal
  /// @param x Sampling time in ns
  void setSampling(const double x) {
    m_Sampling = x;
    m_SignalPoints = m_SignalLength / m_Sampling;
  }

  /// @brief Set length of the signa in ns
  /// @param x Signal length in ns
  constexpr void setSignalLength(const double x) {
    m_SignalLength = x;
    m_SignalPoints = m_SignalLength / m_Sampling;
  }

  /// @brief Set rising time constant of signal @sa SiPMSensor::signalShape
  /// @param x Signal risign time constant in ns
  constexpr void setRiseTime(const double x) {
    m_RiseTime = x;
    m_HasCustomSignalShape = false;
  }

  /// @brief Set falling time constant of signal @sa SiPMSensor::signalShape
  /// @param x Signal falling time constant for fast component in ns
  constexpr void setFallTimeFast(const double x) {
    m_FallTimeFast = x;
    m_HasCustomSignalShape = false;
  }

  /// @brief Set falling time constant for the slow component of signal @sa
  /// SiPMSensor::signalShape
  /// @param x Signal falling time constant for slow component in ns
  constexpr void setFallTimeSlow(const double x) {
    m_FallTimeSlow = x;
    m_HasSlowComponent = true;
    m_HasCustomSignalShape = false;
  }

  /// @brief Set weigth of slow component in the signal
  /// @param x Weight of slow component in the signa. Must be a value in range [0,1]
  constexpr void setSlowComponentFraction(const double x) {
    m_SlowComponentFraction = x;
    m_HasSlowComponent = true;
  }

  /// @brief Set recovery time of the SiPM cell
  /// @param x Recovery time constant of each SiPM cell in ns
  constexpr void setRecoveryTime(const double x) { m_RecoveryTime = x; }

  /// @brief Set SNR value in dB
  /// @param x Signal to noise ratio in dB
  constexpr void setSnr(const double x) {
    m_SnrdB = x;
    m_SnrLinear = pow(10, -m_SnrdB / 20);
  }

  /// @brief Set time constant for the delay of fast afterpulses
  /// @param x Time constant of fast component of afterpulses
  constexpr void setTauApFastComponent(const double x) { m_TauApFastComponent = x; }

  /// @brief Set time constant for the delay of slow afterpulses
  /// @param x Time constant of slow component of afterpulses
  constexpr void setTauApSlowComponent(const double x) { m_TauApSlowComponent = x; }

  /// @brief Set probability to have slow afterpulses over fast ones
  /// @param x Fraction of afterpulses generated using slow component
  constexpr void setApSlowFraction(const double x) { m_ApSlowFraction = x; }

  /// @brief Set cell-to-cell gain variation @sa m_Ccgv
  /// @param x Value of ccgv as a fraction of signal
  constexpr void setCcgv(const float x) { m_Ccgv = x; }

  /// @brief Set value for PDE (and sets @ref PdeType::kSimplePde)
  /// @param x Flat value of PDE to be applied
  constexpr void setPde(const double x) {
    m_Pde = x;
    m_HasPde = PdeType::kSimplePde;
  }

  /// @brief Set dark counts rate
  /// @param val Dark counts rate in Hz
  constexpr void setDcr(const double val) {
    m_Dcr = val;
    m_HasDcr = true;
  }

  /// @brief Set optical crosstalk probability
  /// @param val optical crosstalk probability [0-1]
  constexpr void setXt(const double val) {
    m_Xt = val;
    m_HasXt = true;
  }

  /// @brief Set delayed optical crosstalk probability as a fraction of total xt probability
  /// @param val delayed optical crosstalk probability [0-1]
  constexpr void setDXt(const double val) {
    m_DXt = val;
    m_HasDXt = true;
  }

  /// @brief Set tau of delayed optical crosstalk in ns
  /// @param val tau of delayed optical crosstalk
  constexpr void setDXtTau(const double val) { m_DXtTau = val; }

  /// @brief Set afterpulse probability
  /// @param val afterpulse probability [0-1]
  constexpr void setAp(const double val) {
    m_Ap = val;
    m_HasAp = true;
  }

  /// @brief Turn off dark counts
  constexpr void setDcrOff() { m_HasDcr = false; }
  /// @brief Turn off optical crosstalk
  constexpr void setXtOff() { m_HasXt = false; }
  /// @brief Turn off delayed optical crosstalk
  constexpr void setDXtOff() { m_HasDXt = false; }
  /// @brief Turn off afterpulses
  constexpr void setApOff() { m_HasAp = false; }
  /// @brief Turns off slow component of the signal
  constexpr void setSlowComponentOff() { m_HasSlowComponent = false; }
  /// @brief Turn on dark counts
  constexpr void setDcrOn() { m_HasDcr = true; }
  /// @brief Turn on optical crosstalk
  constexpr void setXtOn() { m_HasXt = true; }
  /// @brief Turn on delayed optical crosstalk
  constexpr void setDXtOn() { m_HasDXt = true; }
  /// @brief Turn on afterpulses
  constexpr void setApOn() { m_HasAp = true; }
  /// @brief Turns on slow component of the signal
  constexpr void setSlowComponentOn() { m_HasSlowComponent = true; }
  /// @brief Sets a different type of PDE simulation @ref PdeType
  /// @param val Type of PDE calculation
  constexpr void setPdeType(PdeType val) { m_HasPde = val; }
  /// @brief Set a spectral response of the SiPM and sets @ref PdeType::kSpectrumPde
  /// The points are resampled on a uniform grid of 32 wavelengths between the first and the last one, interpolating in log space;
  /// segments touching a zero PDE give zero.
  /// @param wav Wavelengths of the points (same unit as the photon wavelengths)
  /// @param pde PDE of each point, same size as @p wav
  void setPdeSpectrum(const std::vector<double>& wav, const std::vector<double>& pde);

  /// @brief Set the distribution of hits on the sensor surface
  /// @param val The @ref HitDistribution to use
  constexpr void setHitDistribution(const HitDistribution val) { m_HitDistribution = val; }

  /// @brief Prints all the properties
  friend std::ostream& operator<<(std::ostream&, const SiPMProperties&);
  /// @brief Description of the properties
  /// @return String printed by operator<<
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

private:
  double m_Size = 1;  ///< Side of the sensitive area in mm
  double m_Pitch = 25;  ///< Side of a cell in um
  uint32_t m_Ncells;  ///< Total number of cells
  uint32_t m_SideCells;  ///< Cells per side
  HitDistribution m_HitDistribution = HitDistribution::kUniform;  ///< Distribution of hits on the surface

  double m_Sampling = 1;  ///< Sampling time in ns
  double m_SignalLength = 500;  ///< Signal length in ns
  uint32_t m_SignalPoints = 0;  ///< Number of samples
  double m_RiseTime = 1;  ///< Rise time constant in ns
  double m_FallTimeFast = 50;  ///< Fast fall time constant in ns
  double m_FallTimeSlow = 100;  ///< Slow fall time constant in ns
  double m_SlowComponentFraction = 0.2;  ///< Weight of the slow component
  double m_RecoveryTime = 50;  ///< Cell recovery time constant in ns

  double m_Dcr = 200e3;  ///< Dark count rate in Hz
  double m_Xt = 0.05;  ///< Optical crosstalk probability
  double m_DXt = 0.05;  ///< Fraction of delayed crosstalk
  double m_DXtTau = 15;  ///< Delayed crosstalk time constant in ns
  double m_Ap = 0.03;  ///< Afterpulse probability
  double m_TauApFastComponent = 10;  ///< Fast afterpulse time constant in ns
  double m_TauApSlowComponent = 80;  ///< Slow afterpulse time constant in ns
  double m_ApSlowFraction = 0.5;  ///< Fraction of slow afterpulses
  float m_Ccgv = 0.05;  ///< Cell-to-cell gain variation (relative sigma)
  double m_SnrdB = 30;  ///< SNR in dB
  float m_Gain = 1.0;  ///< Amplitude of the one photoelectron pulse
  double m_SnrLinear;  ///< Baseline noise sigma

  double m_Pde = 1;  ///< Flat PDE for kSimplePde
  std::map<double, double> m_PdeSpectrum;  ///< Wavelength -> PDE table for kSpectrumPde
  PdeType m_HasPde = PdeType::kNoPde;  ///< Type of PDE calculation

  bool m_HasDcr = true;  ///< Dark counts enabled
  bool m_HasXt = true;  ///< Crosstalk enabled
  bool m_HasDXt = false;  ///< Delayed crosstalk enabled
  bool m_HasAp = true;  ///< Afterpulses enabled
  bool m_HasSlowComponent = false;  ///< Slow component of the pulse enabled
  bool m_HasCustomSignalShape = false;  ///< A user-supplied shape is active
};
} // namespace sipm
#endif /* SIPM_SIPMPROPERTIES_H  */
