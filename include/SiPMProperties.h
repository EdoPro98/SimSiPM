/** @class sipm::SiPMProperties SimSiPM/SimSiPM/SiPMProperties.h
 * SiPMDigitalSignal.h
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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
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
    kCircle,  ///< 95% of photons are uniformly distributed on a circle
    kGaussian ///< 95% of photons have a gaussian distribution
  };

  /// @brief Used to read settings from a json file
  static SiPMProperties readSettings(const std::string&);

  /// @brief Returns size of sensor in mm
  constexpr uint32_t size() const { return m_Size; }

  /// @brief Returns pitch of cell in um
  constexpr uint32_t pitch() const { return m_Pitch; }

  /// @brief Returns total number of cells in the sensor
  constexpr uint32_t nCells() const;

  /// @brief Returns number of cells in the side of the sensor
  constexpr uint32_t nSideCells() const;

  /// @brief Returns total number of points in the signal
  constexpr uint32_t nSignalPoints() const;

  /// @brief Returns @ref HitDistribution type of the sensor
  constexpr HitDistribution hitDistribution() const { return m_HitDistribution; }

  /// @brief Returns total signal length in ns
  constexpr double signalLength() const { return m_SignalLength; }

  /// @brief Returns sampling time considered by the sensor in ns
  constexpr double sampling() const { return m_Sampling; }

  /// @brief Returns rising time constant @sa SiPMSensor::signalShape
  constexpr double risingTime() const { return m_RiseTime; }

  /// @brief Returns falling time constant @sa SiPMSensor::signalShape
  constexpr double fallingTimeFast() const { return m_FallTimeFast; }

  /// @brief Returns falling time constant of slow component @sa
  /// SiPMSensor::signalShape
  constexpr double fallingTimeSlow() const { return m_FallTimeSlow; }

  /// @brief Returns weight of slow component of the signal @sa
  /// SiPMSensor::signalShape.
  constexpr double slowComponentFraction() const { return m_SlowComponentFraction; }

  /// @brief Returns recovery time of SiPM cells.
  constexpr double recoveryTime() const { return m_RecoveryTime; }

  /// @brief Returns DCR value.
  constexpr double dcr() const { return m_Dcr; }

  /// @brief Returns XT value.
  constexpr double xt() const { return m_Xt; }

  /// @brief Returns Delayed XT value.
  constexpr double dxt() const { return m_DXt; }

  /// @brief Returns Delayed XT tau.
  constexpr double dxtTau() const { return m_DXtTau; }

  /// @brief Returns AP value.
  constexpr double ap() const { return m_Ap; }

  /// @brief Returns fast time constant for AP.
  constexpr double tauApFast() const { return m_TauApFastComponent; }

  /// @brief Returns slow time constant for AP.
  constexpr double tauApSlow() const { return m_TauApSlowComponent; }

  /// @brief Returns fraction of AP generated as slow.
  constexpr double apSlowFraction() const { return m_ApSlowFraction; }

  /// @brief Returns value of cell-to-cell gain variation.
  constexpr double ccgv() const { return m_Ccgv; }

  /// @brief Returns relative gain.
  constexpr double gain() const { return m_Gain; }

  /// @brief Returns SNR in dB.
  constexpr double snrdB() const { return m_SnrdB; }

  /// @brief Returns RMS of the noise.
  constexpr double snrLinear() const;

  /// @brief Returns value of PDE if PdeType::kSimplePde is set.
  constexpr double pde() const { return m_Pde; }

  /// @brief Returns wavelength-PDE values if PdeType::kSpectrumPde is set
  const std::map<double, double>& pdeSpectrum() const { return m_PdeSpectrum; }

  /// @brief Returns type of PDE calculation used.
  constexpr PdeType pdeType() { return m_HasPde; }

  /// @brief Returns true if DCR is considered.
  constexpr bool hasDcr() const { return m_HasDcr; }

  /// @brief Returns true if XT is considered.
  constexpr bool hasXt() const { return m_HasXt; }

  /// @brief Returns true if Delayes XT is considered.
  constexpr bool hasDXt() const { return m_HasDXt; }

  /// @brief Returns true if AP is considered.
  constexpr bool hasAp() const { return m_HasAp; }

  /// @brief Returns true if slow component of the signal is considered.
  /// @sa SiPMSensor::signalShape
  constexpr bool hasSlowComponent() const { return m_HasSlowComponent; }

  /// @brief Sets a property using its name
  void setProperty(const std::string&, const double);

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

  /// @brief Set sampling time of the signal in ns
  void setSampling(const double);

  /// @brief Set length of the signa in ns
  /// @parame x Signal length in ns
  constexpr void setSignalLength(const double x) { m_SignalLength = x; }

  /// @brief Set rising time constant of signal @sa SiPMSensor::signalShape
  /// @param x Signal risign time constant in ns
  constexpr void setRiseTime(const double x) { m_RiseTime = x; }

  /// @brief Set falling time constant of signal @sa SiPMSensor::signalShape
  /// @param x Signal falling time constant for fast component in ns
  constexpr void setFallTimeFast(const double x) { m_FallTimeFast = x; }

  /// @brief Set falling time constant for the slow component of signal @sa
  /// SiPMSensor::signalShape
  /// @param x Signal falling time constant for slow component in ns
  constexpr void setFallTimeSlow(const double x) {
    m_FallTimeSlow = x;
    m_HasSlowComponent = true;
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
  constexpr void setTauApSlowFraction(const double x) { m_ApSlowFraction = x; }

  /// @brief Set cell-to-cell gain variation @sa m_Ccgv
  /// @param x Value of ccgv as a fraction of signal
  constexpr void setCcgv(const double x) { m_Ccgv = x; }

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
  constexpr void setDXtOff() { m_HasXt = false; }
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
  constexpr void setPdeType(PdeType val) { m_HasPde = val; }
  /// @brief Set a spectral response of the SiPM and sets @ref
  /// PdeType::kSpectrumPde
  void setPdeSpectrum(const std::vector<double>&, const std::vector<double>&);

  /// @brief Set hit distriution type
  constexpr void setHitDistribution(const HitDistribution val) { m_HitDistribution = val; }

  friend std::ostream& operator<<(std::ostream&, const SiPMProperties&);
  std::string toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

private:
  double m_Size = 1;
  double m_Pitch = 25;
  mutable uint32_t m_Ncells = 0;
  mutable uint32_t m_SideCells = 0;
  HitDistribution m_HitDistribution = HitDistribution::kUniform;

  double m_Sampling = 1;
  double m_SignalLength = 500;
  mutable uint32_t m_SignalPoints = 0;
  double m_RiseTime = 1;
  double m_FallTimeFast = 50;
  double m_FallTimeSlow = 100;
  double m_SlowComponentFraction;
  double m_RecoveryTime = 50;

  double m_Dcr = 200e3;
  double m_Xt = 0.05;
  double m_DXt = 0.05;
  double m_DXtTau = 15;
  double m_Ap = 0.03;
  double m_TauApFastComponent = 10;
  double m_TauApSlowComponent = 80;
  double m_ApSlowFraction = 0.5;
  double m_Ccgv = 0.05;
  double m_SnrdB = 30;
  double m_Gain = 1.0;
  mutable double m_SnrLinear = 0;

  double m_Pde = 1;
  std::map<double, double> m_PdeSpectrum;
  PdeType m_HasPde = PdeType::kNoPde;

  bool m_HasDcr = true;
  bool m_HasXt = true;
  bool m_HasDXt = false;
  bool m_HasAp = true;
  bool m_HasSlowComponent = false;
};
// Constexpr assumes inline

constexpr uint32_t SiPMProperties::nCells() const {
  // m_SideCells and m_Ncells are cached
  if ((m_SideCells == 0) || (m_Ncells == 0)) {
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }
  return m_Ncells;
}

constexpr uint32_t SiPMProperties::nSideCells() const {
  // m_SideCells and m_Ncells are cached
  if ((m_SideCells == 0) || (m_Ncells == 0)) {
    m_SideCells = 1000 * m_Size / m_Pitch;
    m_Ncells = m_SideCells * m_SideCells;
  }
  return m_SideCells;
}

constexpr uint32_t SiPMProperties::nSignalPoints() const {
  // m_Signalpoints is cached
  if (m_SignalPoints == 0) {
    m_SignalPoints = m_SignalLength / m_Sampling;
  }
  return m_SignalPoints;
}

constexpr double SiPMProperties::snrLinear() const {
  // m_SnrLinear is cached
  if (m_SnrLinear == 0) {
    m_SnrLinear = pow(10, -m_SnrdB / 20);
  }
  return m_SnrLinear;
}
} // namespace sipm
#endif /* SIPM_SIPMPROPERTIES_H  */
