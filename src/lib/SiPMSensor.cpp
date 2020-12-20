#include "SiPMSensor.h"
#include <iostream>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <iostream>


namespace sipm{

// Public
// Ctor
SiPMSensor::SiPMSensor(const SiPMProperties& aProperty){
  m_Properties = aProperty;
}


const std::vector<double> SiPMSensor::signalShape()const{
  std::vector<double> lSignalShape(m_Properties.nSignalPoints());

  double sampling = m_Properties.sampling();
  double tr = m_Properties.risingTime() / sampling;
  double tff = m_Properties.fallingTimeFast() / sampling;

  if (m_Properties.hasSlowComponent()){
    double tfs = m_Properties.fallingTimeSlow() / sampling;
    double slf = m_Properties.slowComponentFraction() / sampling;

    for (int32_t i=0; i < m_Properties.nSignalPoints(); ++i){
      lSignalShape[i] = (1-slf)*exp(-i/tff) + slf*exp(-i/tfs) - exp(-i/tr);
    }

  } else {
    for (int32_t i=0; i < m_Properties.nSignalPoints(); ++i){
      lSignalShape[i] = exp(-i/tff) - exp(-i/tr);
    }
  }

  const double peak = *std::max_element(lSignalShape.begin(), lSignalShape.end());

  for(uint32_t i=0; i<m_Properties.nSignalPoints(); ++i){
    lSignalShape[i] /= peak;
  }

  return lSignalShape;
}


void SiPMSensor::setProperties(const SiPMProperties& aProp){
  m_Properties = aProp;
  m_SignalShape = signalShape();
}


void SiPMSensor::addPhotons(const std::vector<double> &aTimes,
  const std::vector<double> &aWavelengths){
  resetState();
  m_PhotonTimes = aTimes;
  m_PhotonWavelengths = aWavelengths;
}


void SiPMSensor::addPhotons(const std::vector<double> &aTimes){
  resetState();
  m_PhotonTimes = aTimes;
}


void SiPMSensor::addPhotons(){
  resetState();
}


// Private
const double SiPMSensor::evaluatePde(const double aPhotonWavelength)const{
  return 0;
}


const bool SiPMSensor::isDetected(const double aPde){
  return(aPde==1)?true:Rand()<aPde;
}


const bool SiPMSensor::isInSensor(const uint32_t r, const uint32_t c)const{
  const uint32_t nSideCells = m_Properties.nSideCells();
  return (r * c > 0 )|(r < nSideCells - 1)|(c < nSideCells -1);
}


const std::array<uint32_t,2> SiPMSensor::hitCell()const{
  uint32_t row,col;
  switch (m_Properties.hitDistribution()) {
    case (SiPMProperties::HitDistribution::kUniform):
      row = randInteger(m_Properties.nSideCells() - 1);
      col = randInteger(m_Properties.nSideCells() - 1);
  }

  return std::array<uint32_t,2> {row, col};
}


void SiPMSensor::addPhotoelecrons(){
  const uint32_t nPhotons = m_PhotonTimes.size();
  std::vector<double> pdes(nPhotons,1);
  std::array<uint32_t,2> position;

  switch (m_Properties.hasPde()) {
    case (SiPMProperties::PdeType::kNoPde):
      break;

    case (SiPMProperties::PdeType::kSimplePde):
      std::fill(pdes.begin(), pdes.end(), m_Properties.pde());
      break;

    case (SiPMProperties::PdeType::kSpectrumPde):
      for (uint32_t i=0; i < nPhotons; ++i){
        pdes[i] = evaluatePde(m_PhotonWavelengths[i]);
      }
      break;
  } /* SWITCH */

  for (uint32_t i = 0; i < nPhotons; ++i){
    if (isDetected(pdes[i])){
      position = hitCell();

      m_Hits.emplace_back(m_PhotonTimes[i],
       1,
       position[0],
       position[1],
       SiPMHit::HitType::kPhotoelectron);

      m_nTotalHits++;
      m_nPe++;
    }
  }
}


void SiPMSensor::addDcrEvents(){
  const double signalLength = m_Properties.signalLength();
  const double meanDcr = 1e9 * m_Properties.dcrTau();
  const uint32_t nSideCells = m_Properties.nSideCells();

  double last = 0;
  uint32_t row, col;

  while (last < signalLength){
    last += randExponential(meanDcr);
    if (last < signalLength){
      row = randInteger(nSideCells - 1);
      col = randInteger(nSideCells - 1);

      m_Hits.emplace_back(last,
       1,
       row,
       col,
       SiPMHit::HitType::kDarkCount);

      m_nTotalHits++;
      m_nDcr++;
    }
  }
}


void SiPMSensor::addXtEvents(){
  const double xt = m_Properties.xt();
  const uint32_t nSideCells = m_Properties.nSideCells();

  uint32_t nxt;
  uint32_t xtGeneratorRow, xtGeneratorCol;
  int32_t xtRow, xtCol;
  double xtTime;

  uint32_t currentCell = 0;
  // Cannotuse iterator based loop becouse of reallocation

  while (currentCell < m_nTotalHits){
    SiPMHit* hit = &m_Hits[currentCell];
    currentCell++;
    nxt = randPoisson(xt);

    if (nxt == 0){continue;}
    xtGeneratorRow = hit->row();
    xtGeneratorCol = hit->col();
    xtTime = hit->time();

    for (uint32_t j = 0; j < nxt; ++j){
      xtRow = xtGeneratorRow + randInteger(2) - 1;
      xtCol = xtGeneratorCol + randInteger(2) - 1;

      if (isInSensor(xtRow, xtCol)){
        m_Hits.emplace_back(
         xtTime,
         1,
         xtRow,
         xtCol,
         SiPMHit::HitType::kOpticalCrosstalk);
        m_nTotalHits++;
        m_nXt++;
      }
    } /* FOR XT */
  } /* WHILE HIT */
}


void SiPMSensor::addApEvents(){
  const double ap = m_Properties.ap();
  const double tauApFast = m_Properties.tauApFast();
  const double tauApSlow = m_Properties.tauApSlow();
  const double signalLength = m_Properties.signalLength();
  const double recoveryTime = m_Properties.recoveryTime();
  const double slowFraction = m_Properties.apSlowFraction();

  uint32_t nap, apGeneratorCol, apGeneratorRow;
  double apDelay, apAmplitude, apGeneratorTime;

  uint32_t currentCell = 0;

  // Cannot use iterator based loop becouse of reallocation
  while (currentCell < m_nTotalHits){
    SiPMHit* hit = &m_Hits[currentCell];
    currentCell++;

    nap = randPoisson(ap);

    if (nap == 0){continue;}
    apGeneratorTime = hit->time();
    apGeneratorRow = hit->row();
    apGeneratorCol = hit->col();

    // Choose fast or slow ap
    for (uint32_t j = 0; j < nap; j++){
      if (Rand() < slowFraction){
        apDelay = randExponential(tauApSlow);
      } else {
        apDelay = randExponential(tauApFast);
      }

      // If ap event is in signal window
      if (apGeneratorTime + apDelay < signalLength){
        apAmplitude = 1 - exp(-apDelay / recoveryTime);

        m_Hits.emplace_back(apGeneratorTime + apDelay,
         apAmplitude,
         apGeneratorRow,
         apGeneratorCol,
         SiPMHit::HitType::kAfterPulse);

        m_nTotalHits++;
        m_nAp++;
      }
    } /* FOR AP */
  } /* WHILE HIT */
}


const std::pair<std::vector<uint32_t>,std::unordered_set<uint32_t>>
 SiPMSensor::getUniqueId()const{
  std::vector<uint32_t> cellId;

  for (auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit){
    cellId.emplace_back(hit->id());
  }

  std::unordered_set<uint32_t> uniqueCellId(cellId.begin(), cellId.end());
  return std::make_pair(cellId, uniqueCellId);
}


void SiPMSensor::calculateSignalAmplitudes(){
  sortHits();
  const auto unique = getUniqueId();
  const std::vector<uint32_t> cellId = std::get<0>(unique);
  const std::unordered_set<uint32_t> uniqueCellId = std::get<1>(unique);
  const double cellRecovery = m_Properties.recoveryTime();

  double delay, previousHitTime;

  for (auto itr = uniqueCellId.begin(); itr != uniqueCellId.cend(); ++itr){
    // If cell hitted more than once
    if (std::count(cellId.begin(), cellId.end(), *itr) > 1){

      previousHitTime = 0;
      for(auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit){

        if(hit->id() == *itr){
          if (previousHitTime == 0){
            previousHitTime = hit->time();
          } else {
            delay = previousHitTime - hit->time();
            hit->amplitude() = 1 - exp(delay / cellRecovery);
            previousHitTime = hit->time();
          }
        }
      }
    }
  }

}


void SiPMSensor::generateSignal(){
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  uint32_t time;
  double amplitude;

  for (auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit){
    time = static_cast<uint32_t>(hit->time() / m_Properties.sampling());
    amplitude = hit->amplitude() * randGaussian(1, m_Properties.ccgv());

    for (uint32_t j = time; j < nSignalPoints; ++j){
      m_Signal[j] += m_SignalShape[j - time] * amplitude;
    }
  }
}


void SiPMSensor::runEvent(){
  m_Signal = randGaussian(0, m_Properties.snrLinear(),
   m_Properties.nSignalPoints());
  m_Signal.setSampling(m_Properties.sampling());

  if (m_Properties.hasDcr()){
    addDcrEvents();
  }

  addPhotoelecrons();

  if (m_Properties.hasXt()){
    addXtEvents();
  }
  calculateSignalAmplitudes();

  if (m_Properties.hasAp()){
    addApEvents();
  }

  if (m_nTotalHits > 0){
    m_SignalShape = signalShape();
    generateSignal();
  }

}


void SiPMSensor::resetState(){
  m_nTotalHits = 0;
  m_nPe = 0;
  m_nDcr = 0;
  m_nXt = 0;
  m_nAp = 0;

  m_Hits.clear();
  m_PhotonTimes.clear();
  m_PhotonWavelengths.clear();
}

} /* NAMESPACE_SIPM */
