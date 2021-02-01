#include "SiPMHit.h"

namespace sipm {
/**
@param aTime        Time of the hit in ns
@param aAmplitude   Amplitude of the hit
@param aRow         Row of the hitted cell
@param aCol         Column of the hitted cell
@param aHitType     Hit type (PE,DCR,XT,...)
*/
SiPMHit::SiPMHit(double aTime, double aAmplitude, uint32_t aRow, uint32_t aCol, const HitType aHitType) noexcept
    : m_Time(aTime),
      m_Amplitude(aAmplitude),
      m_Row(aRow),
      m_Col(aCol),
      m_HitType(aHitType),
      m_Id(makePair(aRow, aCol)) {}
}  // namespace sipm
