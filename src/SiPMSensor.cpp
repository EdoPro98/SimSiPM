#include "SiPMSensor.h"

namespace sipm {

SiPMSensor::SiPMSensor() noexcept {
  m_Properties = SiPMProperties();
  m_Signal.setSampling(m_Properties.sampling());
  m_SignalShape = signalShape();
}

SiPMSensor::SiPMSensor(const SiPMProperties& aProperty) noexcept {
  m_Properties = aProperty;
  m_Signal.setSampling(m_Properties.sampling());
  m_SignalShape = signalShape();
}

void SiPMSensor::setProperty(const std::string& prop, const double val) {
  m_Properties.setProperty(prop, val);
  // After setting property update sipm members
  m_Signal.setSampling(m_Properties.sampling());
  m_SignalShape = signalShape();
}

void SiPMSensor::setProperties(const SiPMProperties& x) {
  m_Properties = x;
  // After setting property update sipm members
  m_Signal.setSampling(m_Properties.sampling());
  m_SignalShape = signalShape();
}

void SiPMSensor::addPhoton(const double aTime) { m_PhotonTimes.emplace_back(aTime); }

void SiPMSensor::addPhoton(const double aTime, const double aWavelength) {
  m_PhotonTimes.emplace_back(aTime);
  m_PhotonWavelengths.emplace_back(aWavelength);
}

void SiPMSensor::addPhotons(const std::vector<double>& aTimes) { m_PhotonTimes = aTimes; }

void SiPMSensor::addPhotons(const std::vector<double>& aTimes, const std::vector<double>& aWavelengths) {
  m_PhotonTimes = aTimes;
  m_PhotonWavelengths = aWavelengths;
}

void SiPMSensor::runEvent() {
  if (m_Properties.hasDcr()) {
    addDcrEvents();
  }
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
  m_PhotonTimes.clear();
  m_PhotonWavelengths.clear();
  m_Signal.clear();
}

std::vector<double> SiPMSensor::signalShape() const {
  const int32_t nSignalPoints = m_Properties.nSignalPoints();
  const double sampling = m_Properties.sampling();
  const double tr = m_Properties.risingTime() / sampling;
  const double tff = m_Properties.fallingTimeFast() / sampling;
  const double gain = m_Properties.gain();
  std::vector<double> lSignalShape(nSignalPoints);

  if (m_Properties.hasSlowComponent()) {
    const double tfs = m_Properties.fallingTimeSlow() / sampling;
    const double slf = m_Properties.slowComponentFraction();

    for (int32_t i = 0; i < nSignalPoints; ++i) {
      lSignalShape[i] = (1 - slf) * exp(-i / tff) + slf * exp(-i / tfs) - exp(-i / tr);
    }
  } else {
    for (int32_t i = 0; i < nSignalPoints; ++i) {
      lSignalShape[i] = exp(-i / tff) - exp(-i / tr);
    }
  }

  const double peak = *std::max_element(lSignalShape.begin(), lSignalShape.end());

  for (int32_t i = 0; i < nSignalPoints; ++i) {
    lSignalShape[i] = lSignalShape[i] / peak * gain;
  }

  return lSignalShape;
}

double SiPMSensor::evaluatePde(const double x) const {
  // Linear interpolation
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

  const double weight = (x - it0->first) / (it1->first - it0->first);
  double y = (weight * it1->second) + ((1 - weight) * it0->second);
  return (y < 0) ? 0 : y;
}

bool SiPMSensor::isInSensor(const int32_t r, const int32_t c) const {
  const int32_t nSideCells = m_Properties.nSideCells();
  return (r >= 0) && (c >= 0) && (r < nSideCells) && (c < nSideCells);
}

math::pair<uint32_t> SiPMSensor::hitCell() const {
  math::pair<uint32_t> hit;
  double x, y;
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
        do {
          x = m_rng.Rand() * 2 - 1;      // x in [-1,1]
          y = m_rng.Rand() * 2 - 1;      // y in [-1,1]
        } while ((x * x) + (y * y) > 1); // if in unitary circle
        hit.first = (x + 1) * m_Properties.nSideCells() / 2;
        hit.second = (y + 1) * m_Properties.nSideCells() / 2;
      } else { // Outside
        do {
          x = m_rng.Rand() * 2 - 1;      // x in [-1,1]
          y = m_rng.Rand() * 2 - 1;      // y in [-1,1]
        } while ((x * x) + (y * y) < 1); // if outside in unitary circle
        hit.first = (x + 1) * m_Properties.nSideCells() / 2;
        hit.second = (y + 1) * m_Properties.nSideCells() / 2;
      }
      break;

    // Gaussian distribution centered in the sensor
    case (SiPMProperties::HitDistribution::kGaussian):
      x = m_rng.randGaussian(0, 1);
      y = m_rng.randGaussian(0, 1);

      if (abs(x) < 1.64 && abs(y) < 1.64) { // 95% of samples = 1.64 sigmas
        hit.first = (x + 1.64) * (m_Properties.nSideCells() / 3.28);
        hit.second = (y + 1.64) * (m_Properties.nSideCells() / 3.28);
      } else {
        hit.first = m_rng.randInteger(nSideCells);
        hit.second = m_rng.randInteger(nSideCells);
      }
      break;
  }
  return hit;
}

void SiPMSensor::addDcrEvents() {
  const double signalLength = m_Properties.signalLength();
  const double meanDcr = 1e9 / m_Properties.dcr();
  const int32_t nSideCells = m_Properties.nSideCells();

  // Starting generation "before" the signal window gives better results
  double last = -100;

  while (last < signalLength) {
    last += m_rng.randExponential(meanDcr);
    if ((last < signalLength) && (last > 0)) {
      const uint32_t row = m_rng.randInteger(nSideCells);
      const uint32_t col = m_rng.randInteger(nSideCells);

      m_Hits.emplace_back(last, 1, row, col, SiPMHit::HitType::kDarkCount);
      ++m_nTotalHits;
      ++m_nDcr;
      ++m_nPe;
    }
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

  do {
    xtRow = row + m_rng.randInteger(2) - 1;
    xtCol = col + m_rng.randInteger(2) - 1;
  } while ((xtRow == row) && (xtCol == col) && !isInSensor(xtRow, xtCol)); // Pick a different cell

  // Time is equal to xtGenerator if isDelayed == false, else add random exponential delay
  const double xtTime = xtGen.time() + m_rng.randExponential(m_Properties.dxtTau()) * (int)isDelayed;

  if (isDelayed) {
    return SiPMHit(xtTime, 1, xtRow, xtCol, SiPMHit::HitType::kDelayedOpticalCrosstalk, xtGen);
  }
  return SiPMHit(xtTime, 1, xtRow, xtCol, SiPMHit::HitType::kOpticalCrosstalk, xtGen);
}

SiPMHit SiPMSensor::generateApHit(const SiPMHit& apGen) const {
  const bool isSlow = m_rng.Rand() < m_Properties.apSlowFraction();

  // If isSlow fast component is multiplied by 0 else slow component is multiplied by 0
  const double delay = m_rng.randExponential(m_Properties.tauApFast()) * (int)(!isSlow) +
                       m_rng.randExponential(m_Properties.tauApSlow()) * (int)isSlow;
  if (isSlow) {
    return SiPMHit(apGen.time() + delay, 1, apGen.row(), apGen.col(), SiPMHit::HitType::kSlowAfterPulse, apGen);
  }
  return SiPMHit(apGen.time() + delay, 1, apGen.row(), apGen.col(), SiPMHit::HitType::kFastAfterPulse, apGen);
}

void SiPMSensor::addCorrelatedNoise() {
  // Correct xt considering multiple xt chains (geometric series)
  const double xtExpMu = exp(-(m_Properties.xt() / (1 + m_Properties.xt())));
  const double apExpMu = exp(-m_Properties.ap() / (1 + m_Properties.ap()));
  const bool hasXt = m_Properties.hasXt();
  const bool hasAp = m_Properties.hasAp();
  const double signalLength = m_Properties.signalLength();

  uint32_t currentHitIdx = 0;
  while (currentHitIdx < m_nTotalHits) {
    // Variable used for poisson process
    double xtPoiss = m_rng.Rand();
    double apPoiss = m_rng.Rand();

    // XT
    while ((xtPoiss > xtExpMu) && hasXt) {
      // Generate generic xt hit
      const SiPMHit xtHit = generateXtHit(m_Hits[currentHitIdx]);
      // Check if hit is valid
      if (isInSensor(xtHit.row(), xtHit.col()) && xtHit.time() < signalLength) {
        // Add hit and increase counters
        m_Hits.push_back(xtHit);
        m_nTotalHits++;
        m_Hits[currentHitIdx].addChildren(xtHit);
        m_nXt++;
        m_nPe++;
        // Increase only if is delayed xt
        m_nDXt += (int)(xtHit.hitType() == SiPMHit::HitType::kDelayedOpticalCrosstalk);
      }
      xtPoiss *= m_rng.Rand();
    }

    // AP
    while ((apPoiss > apExpMu) && hasAp) {
      // Generate generic ap hit
      const SiPMHit apHit = generateApHit(m_Hits[currentHitIdx]);
      // Check if hit is valid
      if (apHit.time() < signalLength) {
        // Add hit and increase counters
        m_Hits.push_back(generateApHit(m_Hits[currentHitIdx]));
        m_nTotalHits++;
        m_Hits[currentHitIdx].addChildren(apHit);
        m_nAp++;
        // m_nPe++; TODO: Should we consider ap as photoelectron?
      }
      apPoiss *= m_rng.Rand();
    }
    // Go to next it till end
    ++currentHitIdx;
  }
}

void SiPMSensor::calculateSignalAmplitudes() {
  // Hits are sorted inplace such that thay have increasing times
  std::sort(m_Hits.begin(), m_Hits.end());
  const double recoveryRate = 1 / m_Properties.recoveryTime();

  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    // Check if cell is fired more than once (at previous times)
    // Since hits are sorted no need to check hits after begin()+i which is current hit
    if (std::count(m_Hits.begin(), m_Hits.begin() + i, m_Hits[i]) > 0) {
      // If so check which hits are in same cell (at previous times)
      for (uint32_t j = 0; j < i; ++j) {
        if (m_Hits[i] == m_Hits[j]) {
          const double delay = m_Hits[i].time() - m_Hits[j].time();
          m_Hits[i].amplitude() = m_Hits[j].amplitude() * (1 - exp(-delay * recoveryRate));
        }
      }
    }
  }
}

// GCC does not vectorize this part very well
// Use AVX2 intrinic to perform signal generation and loop unrolling
#if (defined __AVX2__ && !defined __clang__)
void SiPMSensor::generateSignal() {
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  // Reciprocal of sampling (avoid divisions later)
  const double recSampling = 1 / m_Properties.sampling();

  // Start with gaussian noise
  m_Signal = m_rng.randGaussian(0, m_Properties.snrLinear(), nSignalPoints);
  // Early exit if there are no hits
  if (m_Hits.size() == 0) {
    return;
  }

  for (const auto& hit : m_Hits) {
    const int32_t time = hit.time() * recSampling;
    const double amplitude = hit.amplitude() * m_rng.randGaussian(1, m_Properties.ccgv());
    const __m256d __amplitude = _mm256_set1_pd(amplitude);

    // Skipping tail of loop (no problem since it will be far from the signal (many taus)
    for (uint32_t i = time; i < nSignalPoints - 16; i += 16) {
      __m256d __signal1 = _mm256_loadu_pd(&m_Signal[i]);
      __m256d __signal2 = _mm256_loadu_pd(&m_Signal[i + 4]);
      __m256d __signal3 = _mm256_loadu_pd(&m_Signal[i + 8]);
      __m256d __signal4 = _mm256_loadu_pd(&m_Signal[i + 12]);
      const __m256d __shape1 = _mm256_loadu_pd(&m_SignalShape[i - time]);
      const __m256d __shape2 = _mm256_loadu_pd(&m_SignalShape[i - time + 4]);
      const __m256d __shape3 = _mm256_loadu_pd(&m_SignalShape[i - time + 8]);
      const __m256d __shape4 = _mm256_loadu_pd(&m_SignalShape[i - time + 12]);
      __signal1 = _mm256_fmadd_pd(__shape1, __amplitude, __signal1);
      __signal2 = _mm256_fmadd_pd(__shape2, __amplitude, __signal2);
      __signal3 = _mm256_fmadd_pd(__shape3, __amplitude, __signal3);
      __signal4 = _mm256_fmadd_pd(__shape4, __amplitude, __signal4);
      _mm256_storeu_pd(&m_Signal[i], __signal1);
      _mm256_storeu_pd(&m_Signal[i + 4], __signal2);
      _mm256_storeu_pd(&m_Signal[i + 8], __signal3);
      _mm256_storeu_pd(&m_Signal[i + 12], __signal4);
    }
  }
}
#else
// Clang on the other hand is pretty good at vectorization and unrolling
void SiPMSensor::generateSignal() {
  const uint32_t nHits = m_Hits.size();
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  // Reciprocal of sampling (avoid division)
  const double recSampling = 1 / m_Properties.sampling();

  // Start with gaussian noise
  m_Signal = m_rng.randGaussian(0, m_Properties.snrLinear(), nSignalPoints);
  if (nHits == 0) {
    return;
  }

  // Temp storage in array for vectorization
  alignas(64) uint32_t times[nHits];
  alignas(64) double amplitudes[nHits];
  for (uint32_t i = 0; i < nHits; ++i) {
    times[i] = m_Hits[i].time() * recSampling;
    amplitudes[i] = m_Hits[i].amplitude() * m_rng.randGaussian(1, m_Properties.ccgv());
  }

  // This loop should be vectorized and unrolled by compiler
  for (uint32_t i = 0; i < nHits; ++i) {
    for (uint32_t j = times[i]; j < nSignalPoints; ++j) {
      m_Signal[j] += m_SignalShape[j - times[i]] * amplitudes[i];
    }
  }
}
#endif

std::ostream& operator<<(std::ostream& out, const SiPMSensor& obj) {
  out << std::setprecision(2) << std::fixed;
  out << "===> SiPM Sensor <===\n";
  out << "Address: " << std::addressof(obj) << "\n";
  out << obj.m_Properties;
  out << obj.debug();
  return out;
}

void SiPMSensor::dumpHits() const {
  std::cout << std::setprecision(2) << std::fixed;
  std::cout << "===> Hits <===\n";
  for (const auto& h : m_Hits) {
    std::cout << h << "\n";
  }
}
} // namespace sipm
