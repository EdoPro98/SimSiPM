#include "SiPMSensor.h"
#include "SiPMAnalogSignal.h"
#include "SiPMRandom.h"
#include <SiPMHit.h>
#include <SiPMMath.h>
#include <SiPMTypes.h>
#include <cstdint>

namespace sipm {
  // All constructors MUST call signalShape
SiPMSensor::SiPMSensor() { m_SignalShape = signalShape(); }

SiPMSensor::SiPMSensor(const SiPMProperties& aProperty) {
  m_Properties = aProperty;
  m_SignalShape = signalShape();
}

// Each time a property is changed signalShape MUST be called
void SiPMSensor::setProperty(const std::string& prop, const double val) {
  m_Properties.setProperty(prop, val);
  // After setting property update sipm members
  m_SignalShape = signalShape();
}

void SiPMSensor::setProperties(const SiPMProperties& val) {
  m_Properties = val;
  // After setting property update sipm members
  m_SignalShape = signalShape();
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
  addDcrEvents();
  addPhotoelectrons();
  addCorrelatedNoise();
  calculateSignalAmplitudes();
  generateSignal();
}

void SiPMSensor::resetState() {
  m_nTotalHits = 0;
  m_nPe = 0;
  m_nDcr = 0;
  m_nXt = 0;
  m_nDXt = 0;
  m_nAp = 0;

  m_Hits.clear();
  m_HitsGraph.clear();
  m_PhotonTimes.clear();
  m_PhotonWavelengths.clear();
  m_Signal.clear();
}

SiPMVector<float> SiPMSensor::signalShape() const {
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  const float sampling = m_Properties.sampling();
  const float tr = m_Properties.risingTime() / sampling;
  const float tff = m_Properties.fallingTimeFast() / sampling;
  const float gain = m_Properties.gain();
  SiPMVector<float> lSignalShape(nSignalPoints);

  float peak = 0;

  if (m_Properties.hasSlowComponent()) {
    const float tfs = m_Properties.fallingTimeSlow() / sampling;
    const float slf = m_Properties.slowComponentFraction();

    for (int32_t i = 0; i < nSignalPoints; ++i) {
      lSignalShape[i] = (1 - slf) * exp(-i / tff) + slf * exp(-i / tfs) - exp(-i / tr);
      if (lSignalShape[i] > peak) {
        peak = lSignalShape[i];
      }
    }
  } else {
    for (int32_t i = 0; i < nSignalPoints; ++i) {
      lSignalShape[i] = exp(-i / tff) - exp(-i / tr);
      if (lSignalShape[i] > peak) {
        peak = lSignalShape[i];
      }
    }
  }

  for (uint32_t i = 0; i < nSignalPoints; ++i) {
    lSignalShape[i] = lSignalShape[i] / peak * gain;
  }

  return lSignalShape;
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

  const double m = (it1->second - it0->second) * math::reciprocal(it1->first - it0->first);
  const double q = it0->second - m * it0->first;
  double newy = m * x + q;
  return (newy < 0) ? 0 : newy;
}

math::pair<uint32_t> SiPMSensor::hitCell() const {
  math::pair<uint32_t> hit;
  // index start from 0. nSidecels = 9 gives 10 cells
  const int32_t nSideCells = m_Properties.nSideCells();
  switch (m_Properties.hitDistribution()) {
    // Uniform on the sensor
    case (SiPMProperties::HitDistribution::kUniform):
      hit.first = m_rng.randInteger(nSideCells);
      hit.second = m_rng.randInteger(nSideCells);
      break;
    // Circle centered in sensor 95% probability in circle
    case (SiPMProperties::HitDistribution::kCircle):
      if (m_rng.Rand() < 0.90) { // In circle
        double x, y;
        do {
          x = m_rng.Rand() * 2 - 1;      // x in [-1,1]
          y = m_rng.Rand() * 2 - 1;      // y in [-1,1]
        } while ((x * x) + (y * y) > 1); // if in unitary circle
        hit.first = (x + 1) * m_Properties.nSideCells() * 0.5;
        hit.second = (y + 1) * m_Properties.nSideCells() * 0.5;
      } else { // Outside
        double x, y;
        do {
          x = m_rng.Rand() * 2 - 1;      // x in [-1,1]
          y = m_rng.Rand() * 2 - 1;      // y in [-1,1]
        } while ((x * x) + (y * y) < 1); // if outside in unitary circle
        hit.first = (x + 1) * m_Properties.nSideCells() * 0.5;
        hit.second = (y + 1) * m_Properties.nSideCells() * 0.5;
      }
      break;

    // Gaussian distribution centered in the sensor
    case (SiPMProperties::HitDistribution::kGaussian):
      const double x = m_rng.randGaussian(0, 1);
      const double y = m_rng.randGaussian(0, 1);

      if (abs(x) < 1.64 && abs(y) < 1.64) { // 95% of samples = 1.64 sigmas
        hit.first = (x + 1.64) * (m_Properties.nSideCells() / 3.28);
        hit.second = (y + 1.64) * (m_Properties.nSideCells() / 3.28);
      } else {
        hit.first = m_rng.randInteger(nSideCells);
        hit.second = m_rng.randInteger(nSideCells);
      }
      break;
  } // switch-case
  return hit;
}

void SiPMSensor::addDcrEvents() {
  if (m_Properties.hasDcr() == false){ return; }
  const double signalLength = m_Properties.signalLength();
  const double meanDcr = 1e9 / m_Properties.dcr();
  const int32_t nSideCells = m_Properties.nSideCells();

  // Starting generation "before" the signal window gives better results
  double last = -meanDcr;

  while (last < signalLength) {
    if (last > 0) {
      // DCR are uniform on sipm surface
      const uint32_t row = m_rng.randInteger(nSideCells);
      const uint32_t col = m_rng.randInteger(nSideCells);

      m_Hits.emplace_back(last, 1, row, col, SiPMHit::HitType::kDarkCount);
      // DCR has no parent
      m_HitsGraph.emplace_back(-1);
      ++m_nTotalHits;
      ++m_nDcr;
      ++m_nPe;
    }
    last += m_rng.randExponential(meanDcr);
  }
}

void SiPMSensor::addPhotoelectrons() {
  const uint32_t nPhotons = m_PhotonTimes.size();
  m_Hits.reserve(nPhotons);

  switch (m_Properties.pdeType()) {
    // Add all photons
    case (SiPMProperties::PdeType::kNoPde):
      for (uint32_t i = 0; i < nPhotons; ++i) {
        const math::pair<uint32_t> position = hitCell();
        m_Hits.emplace_back(m_PhotonTimes[i], 1, position.first, position.second, SiPMHit::HitType::kPhotoelectron);
        m_HitsGraph.emplace_back(-1);
        ++m_nTotalHits;
        ++m_nPe;
      }
      break;

    // Simple pde
    case (SiPMProperties::PdeType::kSimplePde):
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (isDetected(m_Properties.pde())) {
          const math::pair<uint32_t> position = hitCell();
          m_Hits.emplace_back(m_PhotonTimes[i], 1, position.first, position.second, SiPMHit::HitType::kPhotoelectron);
          m_HitsGraph.emplace_back(-1);
          ++m_nTotalHits;
          ++m_nPe;
        }
      }
      break;

    // Evaluate pde based on wavelength
    case (SiPMProperties::PdeType::kSpectrumPde):
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (isDetected(evaluatePde(m_PhotonWavelengths[i]))) {
          const math::pair<uint32_t> position = hitCell();
          m_Hits.emplace_back(m_PhotonTimes[i], 1, position.first, position.second, SiPMHit::HitType::kPhotoelectron);
          m_HitsGraph.emplace_back(-1);
          ++m_nTotalHits;
          ++m_nPe;
        }
      }
      break;
  } /* SWITCH */
}

SiPMHit SiPMSensor::generateXtHit(const SiPMHit& xtGen) const {
  int32_t xtRow, xtCol;
  const int32_t row = xtGen.row();
  const int32_t col = xtGen.col();
  const bool isDelayed = (m_rng.Rand() < m_Properties.dxt()) && m_Properties.hasDXt();
  SiPMHit::HitType hitType = SiPMHit::HitType::kOpticalCrosstalk;

  if (isDelayed) {
    hitType = SiPMHit::HitType::kDelayedOpticalCrosstalk;
  }

  do {
    xtRow = row + m_rng.randInteger(3) - 1;
    xtCol = col + m_rng.randInteger(3) - 1;
  } while ((xtRow == row) && (xtCol == col) && !isInSensor(xtRow, xtCol)); // Pick a different cell

  // Time is equal to xtGenerator if isDelayed == false, else add random exponential delay
  const double xtTime = xtGen.time() + m_rng.randExponential(m_Properties.dxtTau()) * (int)isDelayed;

  return SiPMHit{xtTime, 1, static_cast<uint32_t>(xtRow), static_cast<uint32_t>(xtCol), hitType};
}

SiPMHit SiPMSensor::generateApHit(const SiPMHit& apGen) const {
  const bool isSlow = m_rng.Rand() < m_Properties.apSlowFraction();
  SiPMHit::HitType hitType = SiPMHit::HitType::kFastAfterPulse;

  if (isSlow) {
    hitType = SiPMHit::HitType::kSlowAfterPulse;
  }

  // If isSlow fast component is multiplied by 0 else slow component is multiplied by 0
  const double delay = m_rng.randExponential(m_Properties.tauApFast()) * (int)(!isSlow) +
                       m_rng.randExponential(m_Properties.tauApSlow()) * (int)isSlow;
  return SiPMHit{apGen.time() + delay, 1, apGen.row(), apGen.col(), hitType};
}

void SiPMSensor::addCorrelatedNoise() {
  // Correct xt considering multiple xt chains (geometric series)
  const bool hasXt = m_Properties.hasXt();
  const bool hasAp = m_Properties.hasAp();
  const double xtExpMu = exp(-m_Properties.xt() / (1 + m_Properties.xt()));
  const double apExpMu = exp(-m_Properties.ap() / (1 + m_Properties.ap()));

  uint32_t currentHitIdx = 0;
  while (currentHitIdx < m_nTotalHits) {
    // Variables used for poisson process
    double xtPoiss = m_rng.Rand() * (int)(hasXt);
    double apPoiss = m_rng.Rand() * (int)(hasAp);

    // XT
    while (xtPoiss > xtExpMu) {
      // Generate generic xt hit
      const SiPMHit xtHit = generateXtHit(m_Hits[currentHitIdx]);
      // Add hit and increase counters
      m_Hits.push_back(xtHit);
      m_HitsGraph.emplace_back(currentHitIdx);
      m_nTotalHits++;
      m_nXt++;
      m_nPe++;
      // Increase only if is delayed xt
      m_nDXt += (int)(xtHit.hitType() == SiPMHit::HitType::kDelayedOpticalCrosstalk);
      xtPoiss *= m_rng.Rand();
    }

    // AP
    while (apPoiss > apExpMu) {
      // Generate generic ap hit
      const SiPMHit apHit = generateApHit(m_Hits[currentHitIdx]);

      // Add hit and increase counters
      m_Hits.push_back(apHit);
      m_HitsGraph.emplace_back(currentHitIdx);
      m_nTotalHits++;
      m_nAp++;

      apPoiss *= m_rng.Rand();
    }
    // Go to next hit till end
    ++currentHitIdx;
  }
}

void SiPMSensor::calculateSignalAmplitudes() {
  // Hits are sorted inplace such that thay have increasing times
  std::sort(m_Hits.begin(), m_Hits.end());
  const double recoveryRate = 1 / m_Properties.recoveryTime();

  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    // Add ccgv
    m_Hits[i].amplitude() *= m_rng.randGaussian(1, m_Properties.ccgv());
    // Calculate amplitude of cells fired multiple times
    // Just check cells at previous index wrt i. Cells are sorted by time
    // so just chek at "previous times".
    for (uint32_t j = 0; j < i; ++j) {
      if (m_Hits[i] == m_Hits[j]) {
        const double delay = m_Hits[i].time() - m_Hits[j].time();
        m_Hits[i].amplitude() *= m_Hits[j].amplitude() * (1 - exp(-delay * recoveryRate));
      }
    }
  }
}

void SiPMSensor::generateSignal() {
  const uint32_t nHits = m_Hits.size();
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  // Reciprocal of sampling (avoid division later)
  const float recSampling = 1 / m_Properties.sampling();

  // Start with gaussian noise
  m_Signal = SiPMAnalogSignal(m_rng.randGaussianF<SiPMVector<float>>(0, m_Properties.snrLinear(), nSignalPoints),
                              m_Properties.sampling());
  if (nHits == 0) {
    return;
  }

  // Temp storage in vectors
  SiPMVector<uint32_t> times(nHits);
  SiPMVector<float> amplitudes(nHits);

  // Convert ns in units of samples and round to nearest
  for (uint32_t i = 0; i < nHits; ++i) {
    times[i] = std::round(m_Hits[i].time() * recSampling);
    amplitudes[i] = m_Hits[i].amplitude();
  }

  // This loop should be vectorized and unrolled by compiler
  for (uint32_t i = 0; i < nHits; ++i) {
    for (uint32_t j = times[i]; j < nSignalPoints; ++j) {
      m_Signal[j] += m_SignalShape[j - times[i]] * amplitudes[i];
    }
  }
}

std::ostream& operator<<(std::ostream& out, const SiPMSensor& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Sensor <===\n";
  out << "Address: " << std::addressof(obj) << "\n";
  out << obj.m_Properties;
  out << obj.debug();
  return out;
}
} // namespace sipm
