#include "SiPMHit.h"

namespace sipm{

SiPMHit::SiPMHit(double aTime, double aAmplitude, uint32_t aRow, uint32_t aCol,
   const HitType& aHitType)
  :m_Time(aTime),
  m_Amplitude(aAmplitude),
  m_Row(aRow),
  m_Col(aCol),
  m_HitType(aHitType),
  m_Id(makePair(aRow, aCol)){}
} /* NAMESPACE_SIPM */
