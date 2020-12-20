#include <stdint.h>

#ifndef SIPM_SIPMHITS_H
#define SIPM_SIPMHITS_H

namespace sipm{

class SiPMHit{
public:

  enum class HitType{kPhotoelectron,kDarkCount,kOpticalCrosstalk,kAfterPulse};

  SiPMHit(double, double, uint32_t, uint32_t, const HitType&);
  bool operator<(const SiPMHit& aHit)const{return m_Time < aHit.m_Time;}

  // Getters
  inline const double time() const{return m_Time;}
  inline const uint32_t row() const{return m_Row;}
  inline const uint32_t col() const{return m_Col;}
  inline const uint32_t id() const{return m_Id;}
  inline const double amplitude() const{return m_Amplitude;}
  inline double& amplitude() {return m_Amplitude;}
  const HitType& hitType() const{return m_HitType;}

private:
  inline static uint32_t makePair(uint32_t x, uint32_t y){
    return 0.5*(x + y + 1)*(x + y)+y;
  }

  int32_t m_Row, m_Col;
  double m_Time;
  HitType m_HitType;
  uint32_t m_Id;
  double m_Amplitude;
};

}
#endif /* SIPM_SIPMHITS_H */
