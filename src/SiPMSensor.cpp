#include "SiPMSensor.h"
#include "SiPMAnalogSignal.h"
#include "SiPMHit.h"
#include "SiPMProperties.h"
#include "SiPMRandom.h"
#include "SiPMTypes.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <unordered_map>
#include <vector>

namespace sipm {
// All constructors MUST call signalShape
SiPMSensor::SiPMSensor() { signalShape(); }

SiPMSensor::SiPMSensor(const SiPMProperties& aProperty) : m_Properties(aProperty) { signalShape(); }

// Each time a property is changed signalShape MUST be called
void SiPMSensor::setProperty(const std::string& prop, const double val) {
  m_Properties.setProperty(prop, val);
  // After setting property update sipm members
  signalShape();
}

void SiPMSensor::setProperties(const SiPMProperties& val) {
  m_Properties = val;
  // After setting property update sipm members
  signalShape();
}

void SiPMSensor::addPhoton(const double val) { m_PhotonTimes.emplace_back(val); }

void SiPMSensor::addPhoton(const double val1, const double val2) {
  m_PhotonTimes.emplace_back(val1);
  m_PhotonWavelengths.emplace_back(val2);
}

void SiPMSensor::addPhotons(const std::vector<double>& val) {
  m_PhotonTimes = val;
  m_PhotonWavelengths.clear();
}

void SiPMSensor::addPhotons(const std::vector<double>& val1, const std::vector<double>& val2) {
  m_PhotonTimes = val1;
  m_PhotonWavelengths = val2;
}

void SiPMSensor::runEvent() {
  const std::vector<float> waveform = m_rng.randGaussianF(0.0, m_Properties.snrLinear(), m_Properties.nSignalPoints());
  m_Signal = SiPMAnalogSignal(waveform, m_Properties.sampling());
  addDcrEvents();

  addPhotoelectrons();

  addCorrelatedNoise();
  if(m_nTotalHits > 0){
    calculateSignalAmplitudes();
    generateSignal();
  }
}

void SiPMSensor::resetState() {
  m_nTotalHits = 0;
  m_nPe = 0;
  m_nDcr = 0;
  m_nXt = 0;
  m_nDXt = 0;
  m_nAp = 0;
  for (auto hit : m_Hits) {
    delete hit;
  }
  m_Hits.clear();
  m_PhotonTimes.clear();
  m_PhotonWavelengths.clear();
}

void SiPMSensor::signalShape() {
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  const float sampling = m_Properties.sampling();
  const float tr = m_Properties.risingTime() / sampling;
  const float tff = m_Properties.fallingTimeFast() / sampling;
  const float gain = m_Properties.gain();

  m_SignalShape = std::vector<float>(nSignalPoints, 0.0);


  if (m_Properties.hasSlowComponent()) {
    const float tfs = m_Properties.fallingTimeSlow() / sampling;
    const float slf = m_Properties.slowComponentFraction();

    for (uint32_t i = 0; i < nSignalPoints; ++i) {
      m_SignalShape[i] = (1 - slf) * exp(-(float)i / tff) + slf * exp(-(float)i / tfs) - exp(-(float)i / tr);
    }
  } else {
    for (uint32_t i = 0; i < nSignalPoints; ++i) {
      m_SignalShape[i] = exp(-(float)i / tff) - exp(-(float)i / tr);
    }
  }

  const float peak = *std::max_element(m_SignalShape.begin(), m_SignalShape.end());

  for (uint32_t i = 0; i < nSignalPoints; ++i) {
    m_SignalShape[i] = m_SignalShape[i] / peak * gain;
  }
}

double SiPMSensor::evaluatePde(const double x) const {
  // Linear interpolation of x (wlen) to obtain a new value
  // for y (pde) using a LUT stored in m_Properties
  const std::map<double, double>& pde = m_Properties.pdeSpectrum();
  auto it1 = pde.upper_bound(x);
  if (it1 == pde.end()) {
    --it1;
  }
  if (it1 == pde.begin()) {
    ++it1;
  }
  auto it0 = it1;
  --it0;

  const double m = (it1->second - it0->second) / (it1->first - it0->first);
  const double q = it0->second - m * it0->first;
  double newy = m * x + q;
  return (newy < 0) ? 0 : newy;
}

pair<uint32_t> SiPMSensor::hitUniform() const {
  return m_rng.randInteger2(m_Properties.nSideCells());
}

pair<uint32_t> SiPMSensor::hitCircle() const {
  pair<uint32_t> hit;
  if (m_rng.Rand() < 0.90) { // In circle
    double x, y;
    do {
      x = m_rng.Rand() * 2 - 1; // x in [-1,1]
      y = m_rng.Rand() * 2 - 1; // y in [-1,1]
    } while ((x * x) + (y * y) > 1); // if in unitary circle
    hit.first = (x + 1) * m_Properties.nSideCells() * 0.5;
    hit.second = (y + 1) * m_Properties.nSideCells() * 0.5;
  } else { // Outside
    double x, y;
    do {
      x = m_rng.Rand() * 2 - 1; // x in [-1,1]
      y = m_rng.Rand() * 2 - 1; // y in [-1,1]
    } while ((x * x) + (y * y) < 1); // if outside in unitary circle
    hit.first = (x + 1) * m_Properties.nSideCells() * 0.5;
    hit.second = (y + 1) * m_Properties.nSideCells() * 0.5;
  }
  return hit;
}

pair<uint32_t> SiPMSensor::hitGaussian() const {
  pair<uint32_t> hit;
  const double x = m_rng.randGaussian(0, 1);
  const double y = m_rng.randGaussian(0, 1);

  if (std::abs(x) < 1.64 && std::abs(y) < 1.64) { // 95% of samples = 1.64 sigmas
    hit.first = (x + 1.64) * (m_Properties.nSideCells() / 3.28);
    hit.second = (y + 1.64) * (m_Properties.nSideCells() / 3.28);
  } else {
    hit = hitUniform();
  }
  return hit;
}

pair<uint32_t> SiPMSensor::hitCell() const {
  switch (m_Properties.hitDistribution()) {
  case SiPMProperties::HitDistribution::kUniform:
    return hitUniform();
  case SiPMProperties::HitDistribution::kCircle:
    return hitCircle();
  case SiPMProperties::HitDistribution::kGaussian:
    return hitGaussian();
  }
}

void SiPMSensor::addDcrEvents() {
  if (m_Properties.hasDcr() == false) {
    return;
  }
  const double signalLength = m_Properties.signalLength();
  const double meanDcr = 1e9 / m_Properties.dcr();
  const uint32_t nSideCells = m_Properties.nSideCells();

  // Starting generation "before" the signal window gives better results
  double last = -3*meanDcr;

  while (last < signalLength) {
    if (last > 0){
      // DCR are uniform on sipm surface
      const pair<uint32_t> rowcol = m_rng.randInteger2(nSideCells);

      m_Hits.push_back(new SiPMHit{last, 1, rowcol.first, rowcol.second, SiPMHit::HitType::kDarkCount});
      // DCR has no parent
      ++m_nTotalHits;
      ++m_nDcr;
      ++m_nPe;
    }
    last += m_rng.randExponential(meanDcr);
  }
}

void SiPMSensor::addPhotoelectrons() {
  const double sigLen = m_Properties.signalLength();
  const uint32_t nPhotons = m_PhotonTimes.size();
  const SiPMProperties::PdeType pdeType = m_Properties.pdeType();
  constexpr SiPMHit::HitType photoelectron = SiPMHit::HitType::kPhotoelectron;
  m_Hits.reserve(nPhotons);

  switch (pdeType) {
    case SiPMProperties::PdeType::kNoPde:
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if(m_PhotonTimes[i] < 0 || m_PhotonTimes[i] > sigLen){continue;}
        const pair<uint32_t> position = hitCell();
        m_Hits.push_back(new SiPMHit{m_PhotonTimes[i], 1, position.first, position.second, photoelectron});
        m_nTotalHits++;
        m_nPe++;
      }
      return;
    case SiPMProperties::PdeType::kSimplePde:
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (m_PhotonTimes[i] < 0 || m_PhotonTimes[i] > sigLen) { continue;
        }
        if (m_Properties.pde() > m_rng.Rand()) {
          const pair<uint32_t> position = hitCell();
          m_Hits.push_back(new SiPMHit{m_PhotonTimes[i], 1, position.first, position.second, photoelectron});
          m_nTotalHits++;
          m_nPe++;
        }
      }
      return;
    case SiPMProperties::PdeType::kSpectrumPde:
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (m_PhotonTimes[i] < 0 || m_PhotonTimes[i] > sigLen) { continue; }
        if (evaluatePde(m_PhotonWavelengths[i]) > m_rng.Rand()) {
          const pair<uint32_t> position = hitCell();
          m_Hits.push_back(new SiPMHit{m_PhotonTimes[i], 1, position.first, position.second, photoelectron});
          m_nTotalHits++;
          m_nPe++;
        }
      }
      return;
  }
}

SiPMHit* SiPMSensor::generateXtHit(const SiPMHit* xtGen) const {
  int32_t xtRow, xtCol;
  const int32_t row = xtGen->row();
  const int32_t col = xtGen->col();
  const bool isDelayed = m_Properties.hasDXt() && (m_Properties.dxt() > m_rng.Rand());
  const SiPMHit::HitType hitType = isDelayed ? SiPMHit::HitType::kDelayedOpticalCrosstalk : SiPMHit::HitType::kOpticalCrosstalk;

  do {
    xtRow = row + m_rng.randInteger(3) - 1;
    xtCol = col + m_rng.randInteger(3) - 1;
  } while (((xtRow == row) && (xtCol == col)) || !isInSensor(xtRow, xtCol)); // Pick a different cell

  // Time is equal to xtGenerator if isDelayed == false, else add random exponential delay
  double xtDelay = 0;
  if (isDelayed) {
    do {
      xtDelay = m_rng.randExponential(m_Properties.dxtTau());
    } while (xtGen->time() + xtDelay > m_Properties.signalLength());
  }
  return new SiPMHit{xtGen->time() + xtDelay, 1, (uint32_t)xtRow, (uint32_t)xtCol, hitType, xtGen};
}

SiPMHit* SiPMSensor::generateApHit(const SiPMHit* apGen) const {
  const bool isSlow = m_rng.Rand() < m_Properties.apSlowFraction();
  SiPMHit::HitType hitType = SiPMHit::HitType::kFastAfterPulse;
  if (isSlow) {
    hitType = SiPMHit::HitType::kSlowAfterPulse;
  }

  double delay = 0;
  do {
    delay = m_rng.randExponential(m_Properties.tauApFast());
    if(isSlow){
      delay = m_rng.randExponential(m_Properties.tauApSlow());
    }
  } while (apGen->time() + delay > m_Properties.signalLength());

  return new SiPMHit{apGen->time() + delay, 1, apGen->row(), apGen->col(), hitType, apGen};
}

void SiPMSensor::addCorrelatedNoise() {
  const bool hasXt = m_Properties.hasXt();
  const bool hasAp = m_Properties.hasAp();

  const double xtExpMu = exp(-m_Properties.xt());
  const double apExpMu = exp(-m_Properties.ap());

  for (uint32_t currentHitIdx = 0; currentHitIdx < m_nTotalHits; ++currentHitIdx) {
    // Variables used for poisson process
    double xtPoiss = m_rng.Rand() * (int)(hasXt);
    double apPoiss = m_rng.Rand() * (int)(hasAp);

    // XT
    while (xtPoiss > xtExpMu) {
      // Generate generic xt hit
      SiPMHit* xtHit = generateXtHit(m_Hits[currentHitIdx]);
      // Increase only if is delayed xt
      m_nDXt += (int)(xtHit->hitType() == SiPMHit::HitType::kDelayedOpticalCrosstalk);
      // Add hit and increase counters
      m_Hits.push_back(xtHit);
      m_nTotalHits++;
      m_nXt++;
      m_nPe++;
      // Poisson process
      xtPoiss *= m_rng.Rand();
    }

    // AP
    while (apPoiss > apExpMu) {
      // Generate generic ap hit
      SiPMHit* apHit = generateApHit(m_Hits[currentHitIdx]);

      // Add hit and increase counters
      m_Hits.push_back(apHit);
      m_nTotalHits++;
      m_nAp++;
      // Poisson process
      apPoiss *= m_rng.Rand();
    }
  }
}

void SiPMSensor::calculateSignalAmplitudes() {
  const double recoveryRate = 1 / m_Properties.recoveryTime();

  // Setup an hash table to store hits and counts
  std::unordered_map<uint32_t, SiPMSmallVector<SiPMHit*, 4>> hashTable;
  hashTable.reserve(m_nTotalHits);

  // Add ccgv to all hits
  const std::vector<float> ccgvs = m_rng.randGaussianF(1, m_Properties.ccgv(), m_nTotalHits);
  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    m_Hits[i]->amplitude() *= ccgvs[i];
  }

  // Hits are stored in a hash table. Each key of the table
  // is a sipm cell. Hits in same cell are stored in a vector
  // in same key of table.
  const uint32_t nSideCells = m_Properties.nSideCells();
  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    SiPMHit* hit = m_Hits[i];
    const uint32_t hash = hit->col() + nSideCells * hit->row();
    hashTable[hash].push_back(hit);
  }

  // Iterate over hash table
  for (auto& [hash, hits] : hashTable) {
    const uint32_t n_hits = hits.size();
    // If less than two hit in same cell go ahead
    if (n_hits <= 1) { continue; }
    std::sort(hits.begin(), hits.end(), [](const SiPMHit* a, const SiPMHit* b) { return *a < *b; });
    // Calculate amplitude
    for (uint32_t i = 1; i < n_hits; ++i) {
      const double delay = hits[i]->time() - hits[0]->time();
      hits[i]->amplitude() *= 1 - exp(-delay * recoveryRate);
    }
  }
}


void SiPMSensor::generateSignal() {
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  // Reciprocal of sampling (avoid division later)
  const float recSampling = 1 / m_Properties.sampling();

  // Convert ns in units of samples and round to nearest
  std::vector<uint32_t> times(m_nTotalHits);
  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    times[i] = std::floor(m_Hits[i]->time() * recSampling);
  }

  // This loop should be vectorized and unrolled by compiler
  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    const uint32_t time = times[i];
    const float amplitude = m_Hits[i]->amplitude();


    const uint32_t endPoint = nSignalPoints - time;
    float* signalPtr = m_Signal.data() + time;
    const float* signalShapePtr = m_SignalShape.data();

    for (uint32_t j = 0; j < endPoint; ++j) {
      signalPtr[j] += signalShapePtr[j] * amplitude;
    }
  }
}

std::ostream& operator<<(std::ostream& out, const SiPMSensor& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Sensor <===\n";
  out << "Address: " << std::hex << std::addressof(obj) << "\n";
  out << obj.m_Properties;
  out << obj.debug();
  return out;
}
} // namespace sipm
