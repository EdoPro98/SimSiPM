#include "SiPMSensor.h"
#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <iostream>
namespace sipm{

// Public
// Ctor
SiPMSensor::SiPMSensor(const SiPMProperties& aProperty){
  m_Properties = aProperty;
  m_SignalShape = signalShape();
  m_Signal.setSampling(m_Properties.sampling());
}

void SiPMSensor::setProperty(const std::string& prop, const double val){
  m_Properties.setProperty(prop, val);
  m_SignalShape = signalShape();
  m_Signal.setSampling(m_Properties.sampling());
}

void SiPMSensor::setProperties(const std::vector<std::string>& props, const std::vector<double>& vals){
  for (uint32_t i=0; i<props.size(); ++i){
    m_Properties.setProperty(props[i], vals[i]);
  }
}


const std::vector<double> SiPMSensor::signalShape()const{
  std::vector<double> lSignalShape(m_Properties.nSignalPoints());

  double sampling = m_Properties.sampling();
  double tr = m_Properties.risingTime() / sampling;
  double tff = m_Properties.fallingTimeFast() / sampling;

  if (m_Properties.hasSlowComponent()){
    double tfs = m_Properties.fallingTimeSlow() / sampling;
    double slf = m_Properties.slowComponentFraction();

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


void SiPMSensor::addPhotons(const std::vector<double> &aTimes,
  const std::vector<double> &aWavelengths){
  resetState();
  m_PhotonTimes = aTimes;
  m_PhotonWavelengths = aWavelengths;
}


// Private
const double SiPMSensor::evaluatePde(const double aPhotonWavelength)const{
  return 0;
}


const bool SiPMSensor::isInSensor(const int32_t r, const int32_t c)const{
  const int32_t nSideCells = m_Properties.nSideCells();
  return (r * c > 0 )|(r < nSideCells - 1)|(c < nSideCells - 1);
}


const std::array<int32_t,2> SiPMSensor::hitCell()const{
  int32_t row,col;
  switch (m_Properties.hitDistribution()) {
    case (SiPMProperties::HitDistribution::kUniform):
      row = randInteger(m_Properties.nSideCells() - 1);
      col = randInteger(m_Properties.nSideCells() - 1);
      break;
  }

  return std::array<int32_t,2> {row, col};
}


void SiPMSensor::addPhotoelectrons(){
  const uint32_t nPhotons = m_PhotonTimes.size();
  const double pde = m_Properties.pde();
  m_Hits.reserve(nPhotons);

  switch (m_Properties.hasPde()) {

  case (SiPMProperties::PdeType::kNoPde):
    // Add all photons
    for (uint32_t i = 0; i < nPhotons; ++i){
      std::array<int32_t,2> position = hitCell();

      m_Hits.emplace_back(m_PhotonTimes[i],
       1,
       position[0],
       position[1],
       SiPMHit::HitType::kPhotoelectron);

      m_nTotalHits++;
      m_nPe++;
    }
    break;

  case (SiPMProperties::PdeType::kSimplePde):
    // Simple pde
    for (uint32_t i = 0; i < nPhotons; ++i){
      if (isDetected(pde)){
        std::array<int32_t,2> position = hitCell();

        m_Hits.emplace_back(m_PhotonTimes[i],
         1,
         position[0],
         position[1],
         SiPMHit::HitType::kPhotoelectron);

        m_nTotalHits++;
        m_nPe++;
      }
    }
    break;

  case (SiPMProperties::PdeType::kSpectrumPde):
    // Evaluate pde based on wavelength
    for (uint32_t i = 0; i < nPhotons; ++i){
      if (isDetected(evaluatePde(m_PhotonWavelengths[i]))){
        std::array<int32_t,2> position = hitCell();

        m_Hits.emplace_back(m_PhotonTimes[i],
         1,
         position[0],
         position[1],
         SiPMHit::HitType::kPhotoelectron);

        m_nTotalHits++;
        m_nPe++;
      }
    }
    break;
  } /* SWITCH */
}


void SiPMSensor::addDcrEvents(){
  const double signalLength = m_Properties.signalLength();
  const double meanDcr = 1e9 * m_Properties.dcrTau();
  const int32_t nSideCells = m_Properties.nSideCells();

  double last = -100;

  while (last < signalLength){
    last += randExponential(meanDcr);
    if ((last > 0) && (last < signalLength)){
      int32_t row = randInteger(nSideCells);
      int32_t col = randInteger(nSideCells);

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
  const int32_t nSideCells = m_Properties.nSideCells();

  uint32_t currentCellIdx = 0;
  while (currentCellIdx < m_nTotalHits){
    SiPMHit* hit = &m_Hits[currentCellIdx];
    currentCellIdx++;
    uint32_t nxt = randPoisson(xt);

    if (nxt == 0){continue;}
    int32_t xtGeneratorRow = hit->row();
    int32_t xtGeneratorCol = hit->col();
    double xtTime = hit->time();

    for (uint32_t j = 0; j < nxt; ++j){
      int32_t xtRow = xtGeneratorRow + randInteger(2) - 1;
      int32_t xtCol = xtGeneratorCol + randInteger(2) - 1;

      if (isInSensor(xtRow, xtCol)){
        m_Hits.emplace_back(
         xtTime,
         1,
         xtRow,
         xtCol,
         SiPMHit::HitType::kOpticalCrosstalk);
        ++m_nTotalHits;
        ++m_nXt;
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


  // Cannot use iterator based loop becouse of reallocation
  uint32_t currentCellIdx = 0;
  while (currentCellIdx < m_nTotalHits){
    SiPMHit* hit = &m_Hits[currentCellIdx];
    currentCellIdx++;

    int32_t nap = randPoisson(ap);

    if (nap == 0){continue;}
    double apGeneratorTime = hit->time();
    int32_t apGeneratorRow = hit->row();
    int32_t apGeneratorCol = hit->col();

    // Choose fast or slow ap
    for (uint32_t j = 0; j < nap; ++j){
      double apDelay;
      if (Rand() < slowFraction){
        apDelay = randExponential(tauApSlow);
      } else {
        apDelay = randExponential(tauApFast);
      }

      // If ap event is in signal window
      if (apGeneratorTime + apDelay < signalLength){
        double apAmplitude = 1 - exp(-apDelay / recoveryTime);

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
  std::pair<std::vector<uint32_t>,std::unordered_set<uint32_t>> out;

  std::vector<uint32_t> cellId;
  cellId.reserve(m_Hits.size());
  for (auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit){
    cellId.emplace_back(hit->id());
  }
  std::unordered_set<uint32_t> uniqueCellId(cellId.begin(), cellId.end());

  out = std::make_pair(cellId, uniqueCellId);
  return out;
}


void SiPMSensor::calculateSignalAmplitudes(){
  sortHits();
  const auto unique = getUniqueId();
  const std::vector<uint32_t> cellId = std::get<0>(unique);
  const std::unordered_set<uint32_t> uniqueCellId = std::get<1>(unique);
  const double cellRecovery = m_Properties.recoveryTime();

  for (auto itr = uniqueCellId.begin(); itr != uniqueCellId.end(); ++itr){
    // If cell hitted more than once
    if (std::count(cellId.begin(), cellId.end(), *itr) > 1){

      double previousHitTime = 0;
      for(auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit){

        if(hit->id() == *itr){
          if (previousHitTime == 0){
            previousHitTime = hit->time();
          } else {
            double delay = hit->time() - previousHitTime;
            hit->amplitude() = 1 - exp(-delay / cellRecovery);
            previousHitTime = hit->time();
          }
        }
      }
    }
  }

}


void SiPMSensor::generateSignal(){
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();

  m_Signal = randGaussian(0, m_Properties.snrLinear(), nSignalPoints);

  for (auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit){
    const uint32_t time = static_cast<uint32_t>(hit->time() / m_Properties.sampling());
    const double amplitude = hit->amplitude() * randGaussian(1, m_Properties.ccgv());

    for (uint32_t j = time; j < nSignalPoints; ++j){
      m_Signal[j] += m_SignalShape[j - time] * amplitude;
    }
  }
}


void SiPMSensor::runEvent(){

  if (m_Properties.hasDcr()){
    addDcrEvents();
  }

  addPhotoelectrons();

  if (m_Properties.hasXt()){
    addXtEvents();
  }
  calculateSignalAmplitudes();

  if (m_Properties.hasAp()){
    addApEvents();
  }

  generateSignal();
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
