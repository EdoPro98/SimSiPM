#include <stdint.h>
#include <vector>
#include <string>

#ifndef SIPM_SIPMPROPERTIES_H
#define SIPM_SIPMPROPERTIES_H

namespace sipm{

class SiPMProperties{
public:

  enum class PdeType{kNoPde, kSimplePde, kSpectrumPde};
  enum class HitDistribution{kUniform};

  // Read/Write settings
  void readSettings(std::string&);
  void dumpSettings()const;

  // Getters
  const uint32_t nCells()const;
  const uint32_t nSideCells()const;
  const uint32_t nSignalPoints()const;
  const HitDistribution hitDistribution() const{return m_HitDistribution;}
  const double signalLength() const{return m_SignalLength;}
  const double sampling() const{return m_Sampling;}
  const double risingTime() const{return m_RiseTime;}
  const double fallingTimeFast() const{return m_FallTimeFast;}
  const double fallingTimeSlow() const{return m_FallTimeSlow;}
  const double slowComponentFraction() const{return m_SlowComponentFraction;}
  const double recoveryTime() const{return m_RecoveryTime;}
  const double dcr() const{return m_Dcr;}
  const double dcrTau() const{return 1 / m_Dcr;}
  const double xt() const{return m_Xt;}
  const double ap() const{return m_Ap;}
  const double tauApFast() const{return m_TauApFastComponent;}
  const double tauApSlow() const{return m_TauApSlowComponent;}
  const double apSlowFraction() const{return m_ApSlowFraction;}
  const double ccgv() const{return m_Ccgv;}
  const double snrdB() const{return m_SnrdB;}
  const double snrLinear() const;
  const double pde() const{return m_Pde;}
  const std::pair<std::vector<double>, std::vector<double>> pdeSpectrum() const;
  const PdeType hasPde(){return m_HasPde;}
  const bool hasDcr() const{return m_HasDcr;}
  const bool hasXt() const{return m_HasXt;}
  const bool hasAp() const{return m_HasAp;}
  const bool hasSlowComponent() const{return m_HasSlowComponent;}

  // Setters
  void setProperty(const std::string&, const double);

private:
  void setSize(const double x){m_Size = x;}
  void setPitch(const double x){m_Pitch = x;}
  void setSampling(const double x);
  void setSignalLength(const double x){m_SignalLength = x;}
  void setRiseTime(const double x){m_RiseTime = x;}
  void setFallTimeFast(const double x){m_FallTimeFast = x;}
  void setFallTimeSlow(const double x)
  {m_FallTimeSlow = x; m_HasSlowComponent = true;}
  void setSlowComponentFraction(const double x){m_SlowComponentFraction = x;}
  void setRecoveryTime(const double x){m_RecoveryTime = x;}
  void setSnr(const double aSnr){m_SnrdB = aSnr;}
  void setTauApFastComponent(const double x)
  {m_TauApFastComponent = x;m_HasAp = true;}
  void setTauApSlowComponent(const double x)
  {m_TauApFastComponent = x;m_HasAp = true;}
  void setTauApSlowFraction(const double x){m_ApSlowFraction = x;}
  void setTauAp(const double,const  double);
  void setCcgv(const double x){m_Ccgv = x;}
  void setPde(const double x){m_Pde = x; m_HasPde = PdeType::kSimplePde;}
  void setPdeSpectrum(const std::vector<double>& x, const std::vector<double>& y)
  {m_PdeSpectrum = x; m_PhotonWavelength = y; m_HasPde = PdeType::kSpectrumPde;}
  void setDcr(const double x){m_Dcr = x; m_HasDcr = true;}
  void setXt(const double x){m_Xt = x; m_HasXt = true;}
  void setAp(const double x){m_Ap = x; m_HasAp = true;}
  void setDcrOff(){m_HasDcr = false;}
  void setApOff(){m_HasAp = false;}
  void setXtOff(){m_HasXt = false;}
  void setPdeOff(){m_HasPde = PdeType::kNoPde;}
  void setHasSlowComponent(const bool x){m_HasSlowComponent = x;}

  // Geometry
  double m_Size = 1;
  double m_Pitch = 25;
  mutable uint32_t m_SideCells = 0;
  mutable uint32_t m_Ncells = 0;
  HitDistribution m_HitDistribution = HitDistribution::kUniform;

  // Signal
  double m_Sampling = 1;
  double m_SignalLength = 500;
  mutable uint32_t m_SignalPoints = 0;
  double m_RiseTime = 1;
  double m_FallTimeFast = 50;
  double m_FallTimeSlow;
  double m_SlowComponentFraction;
  double m_RecoveryTime = 50;

  // Noise
  double m_Dcr = 200e3;
  double m_Xt = 0.05;
  double m_Ap = 0.03;
  double m_TauApFastComponent = 10;
  double m_TauApSlowComponent = 80;
  double m_ApSlowFraction = 0.8;
  double m_Ccgv = 0.05;
  double m_SnrdB = 30;
  mutable double m_SnrLinear = 0;

  // Pde
  double m_Pde;
  std::vector<double> m_PdeSpectrum;
  std::vector<double> m_PhotonWavelength;
  PdeType m_HasPde = PdeType::kNoPde;

  // Bools
  bool m_HasDcr = true;
  bool m_HasXt = true;
  bool m_HasAp = true;
  bool m_HasSlowComponent = false;
};

} /* NAMESPACE_SIPM */
#endif /* SIPM_SIPMPROPERTIES_H  */
