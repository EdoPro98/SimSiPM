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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <cmath>
#include <stdint.h>
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
  void readSettings(std::string&); ///< @todo Still to implement

  /// @brief Returns size of sensor in mm
  uint32_t size() const { return m_Size; }

  /// @brief Returns pitch of cell in um
  uint32_t pitch() const { return m_Pitch; }

  /// @brief Returns total number of cells in the sensor
  uint32_t nCells() const;

  /// @brief Returns number of cells in the side of the sensor
  uint32_t nSideCells() const;

  /// @brief Returns total number of points in the signal
  uint32_t nSignalPoints() const;

  /// @brief Returns @ref HitDistribution type of the sensor
  HitDistribution hitDistribution() const { return m_HitDistribution; }

  /// @brief Returns total signal length in ns
  double signalLength() const { return m_SignalLength; }

  /// @brief Returns sampling time considered by the sensor in ns
  double sampling() const { return m_Sampling; }

  /// @brief Returns rising time constant @sa SiPMSensor::signalShape
  double risingTime() const { return m_RiseTime; }

  /// @brief Returns falling time constant @sa SiPMSensor::signalShape
  double fallingTimeFast() const { return m_FallTimeFast; }

  /// @brief Returns falling time constant of slow component @sa
  /// SiPMSensor::signalShape
  double fallingTimeSlow() const { return m_FallTimeSlow; }

  /// @brief Returns weight of slow component of the signal @sa
  /// SiPMSensor::signalShape.
  double slowComponentFraction() const { return m_SlowComponentFraction; }

  /// @brief Returns recovery time of SiPM cells.
  double recoveryTime() const { return m_RecoveryTime; }

  /// @brief Returns DCR value.
  double dcr() const { return m_Dcr; }

  /// @brief Returns XT value.
  double xt() const { return m_Xt; }

  /// @brief Returns Delayed XT value.
  double dxt() const { return m_DXt; }

  /// @brief Returns Delayed XT tau.
  double dxtTau() const { return m_DXtTau; }

  /// @brief Returns AP value.
  double ap() const { return m_Ap; }

  /// @brief Returns fast time constant for AP.
  double tauApFast() const { return m_TauApFastComponent; }

  /// @brief Returns slow time constant for AP.
  double tauApSlow() const { return m_TauApSlowComponent; }

  /// @brief Returns fraction of AP generated as slow.
  double apSlowFraction() const { return m_ApSlowFraction; }

  /// @brief Returns value of cell-to-cell gain variation.
  double ccgv() const { return m_Ccgv; }

  /// @brief Returns relative gain.
  double gain() const { return m_Gain; }

  /// @brief Returns SNR in dB.
  double snrdB() const { return m_SnrdB; }

  /// @brief Returns RMS of the noise.
  double snrLinear() const;

  /// @brief Returns value of PDE if PdeType::kSimplePde is set.
  double pde() const { return m_Pde; }

  /// @brief Returns wavelength-PDE values if PdeType::kSpectrumPde is set
  const std::map<double, double>& pdeSpectrum() const { return m_PdeSpectrum; }

  /// @brief Returns type of PDE calculation used.
  PdeType pdeType() { return m_HasPde; }

  /// @brief Returns true if DCR is considered.
  bool hasDcr() const { return m_HasDcr; }

  /// @brief Returns true if XT is considered.
  bool hasXt() const { return m_HasXt; }

  /// @brief Returns true if Delayes XT is considered.
  bool hasDXt() const { return m_HasDXt; }

  /// @brief Returns true if AP is considered.
  bool hasAp() const { return m_HasAp; }

  /// @brief Returns true if slow component of the signal is considered.
  /// @sa SiPMSensor::signalShape
  bool hasSlowComponent() const { return m_HasSlowComponent; }

  /// @brief Sets a property using its name
  void setProperty(const std::string&, const double);

  /// @brief Set size of SiPM sensitive area (side in mm)
  void setSize(const double x) { m_Size = x; }

  /// @brief Set pitch of SiPM cells (side in um)
  void setPitch(const double x) { m_Pitch = x; }

  /// @brief Set sampling time of the signal in ns
  void setSampling(const double x);

  /// @brief Set length of the signa in ns
  void setSignalLength(const double x) { m_SignalLength = x; }

  /// @brief Set rising time constant of signal @sa SiPMSensor::signalShape
  void setRiseTime(const double x) { m_RiseTime = x; }

  /// @brief Set falling time constant of signal @sa SiPMSensor::signalShape
  void setFallTimeFast(const double x) { m_FallTimeFast = x; }

  /// @brief Set falling time constant for the slow component of signal @sa
  /// SiPMSensor::signalShape
  void setFallTimeSlow(const double x) { m_FallTimeSlow = x; }

  /// @brief Set weigth of slow component in the signal
  void setSlowComponentFraction(const double x) { m_SlowComponentFraction = x; }

  /// @brief Set recovery time of the SiPM cell
  void setRecoveryTime(const double x) { m_RecoveryTime = x; }

  /// @brief Set SNR value in dB
  void setSnr(const double x) {
    m_SnrdB = x;
    m_SnrLinear = pow(10, -x / 20);
  }

  /// @brief Set time constant for the delay of fast afterpulses
  void setTauApFastComponent(const double x) { m_TauApFastComponent = x; }

  /// @brief Set time constant for the delay of slow afterpulses
  void setTauApSlowComponent(const double x) { m_TauApSlowComponent = x; }

  /// @brief Set probability to have slow afterpulses over fast ones
  void setTauApSlowFraction(const double x) { m_ApSlowFraction = x; }

  /// @brief Set cell-to-cell gain variation @sa m_Ccgv
  void setCcgv(const double x) { m_Ccgv = x; }

  /// @brief Set value for PDE (and sets @ref PdeType::kSimplePde)
  void setPde(const double x) { m_Pde = x; }

  /// @brief Set dark counts rate
  /// @param aDcr Dark counts rate in Hz
  void setDcr(const double aDcr) { m_Dcr = aDcr; }

  /// @brief Set optical crosstalk probability
  /// @param aXt optical crosstalk probability [0-1]
  void setXt(const double aXt) { m_Xt = aXt; }

  /// @brief Set delayed optical crosstalk probability as a fraction of total xt probability
  /// @param aDXt delayed optical crosstalk probability [0-1]
  void setDXt(const double aDXt) { m_DXt = aDXt; }

  /// @brief Set tau of delayed optical crosstalk in ns
  /// @param aDXt tau of delayed optical crosstalk
  void setDXtTau(const double aDXtTau) { m_DXtTau = aDXtTau; }

  /// @brief Set afterpulse probability
  /// @param aAp afterpulse probability [0-1]
  void setAp(const double aAp) { m_Ap = aAp; }

  /// @brief Turn off dark counts
  void setDcrOff() { m_HasDcr = false; }
  /// @brief Turn off optical crosstalk
  void setXtOff() { m_HasXt = false; }
  /// @brief Turn off delayed optical crosstalk
  void setDXtOff() { m_HasXt = false; }
  /// @brief Turn off afterpulses
  void setApOff() { m_HasAp = false; }
  /// @brief Turns off slow component of the signal
  void setSlowComponentOff() { m_HasSlowComponent = false; }
  /// @brief Turn on dark counts
  void setDcrOn() { m_HasDcr = true; }
  /// @brief Turn on optical crosstalk
  void setXtOn() { m_HasXt = true; }
  /// @brief Turn on delayed optical crosstalk
  void setDXtOn() { m_HasDXt = true; }
  /// @brief Turn on afterpulses
  void setApOn() { m_HasAp = true; }
  /// @brief Turns on slow component of the signal
  void setSlowComponentOn() { m_HasSlowComponent = true; }
  /// @brief Turn off PDE: set @ref PdeType::kNoPde
  void setPdeType(PdeType aPdeType) { m_HasPde = aPdeType; }
  /// @brief Set a spectral response of the SiPM and sets @ref
  /// PdeType::kSpectrumPde
  void setPdeSpectrum(const std::map<double, double>&);
  /// @brief Set a spectral response of the SiPM and sets @ref
  /// PdeType::kSpectrumPde
  void setPdeSpectrum(const std::vector<double>&, const std::vector<double>&);

  void setHitDistribution(const HitDistribution aHitDistribution) { m_HitDistribution = aHitDistribution; }

  friend std::ostream& operator<< (std::ostream&, const SiPMProperties&);

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
  double m_FallTimeSlow;
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

  double m_Pde;
  std::map<double, double> m_PdeSpectrum;
  PdeType m_HasPde = PdeType::kNoPde;

  bool m_HasDcr = true;
  bool m_HasXt = true;
  bool m_HasDXt = false;
  bool m_HasAp = true;
  bool m_HasSlowComponent = false;
};

} // namespace sipm
#endif /* SIPM_SIPMPROPERTIES_H  */
