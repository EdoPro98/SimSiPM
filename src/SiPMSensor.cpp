#include "SiPMSensor.h"

#include "SiPMHelpers.h"
#include "SiPMProperties.h"

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
  m_Signal.setSampling(m_Properties.sampling());
  m_SignalShape = signalShape();
}

void SiPMSensor::setProperties(const SiPMProperties& x) {
  m_Properties = x;
  m_Signal.setSampling(m_Properties.sampling());
  m_SignalShape = signalShape();
}

void SiPMSensor::setPrecisionLevel(const PrecisionLevel x) { m_PrecisionLevel = x; }

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
  if (m_Properties.hasXt()) {
    addXtEvents();
  }

  // TODO: Maybe it is better to generate AP with amplitude 1 and
  // calculate signal amplitudes after AP generation.
  // Still to discuss the physics behind.
  calculateSignalAmplitudes();
  if (m_Properties.hasAp()) {
    addApEvents();
  }

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
  const std::map<double, double> pde = m_Properties.pdeSpectrum();
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
  return (weight * it1->second) + ((1 - weight) * it0->second);
}

bool SiPMSensor::isInSensor(const int32_t r, const int32_t c) const {
  const int32_t nSideCells = m_Properties.nSideCells();
  return (r >= 0) && (c >= 0) && (r < nSideCells) && (c < nSideCells);
}

std::pair<uint32_t, uint32_t> SiPMSensor::hitCell() const {
  uint32_t row, col;
  double x, y;
  const int32_t nSideCells = m_Properties.nSideCells(); // index start from 0. nSidecels = 9 gives 10 cells

  switch (m_Properties.hitDistribution()) {
    // Uniform on the sensor
    case (SiPMProperties::HitDistribution::kUniform):
      row = m_rng.randInteger(nSideCells);
      col = m_rng.randInteger(nSideCells);
      break;

    // Circle centered in sensor 95% probability in circle
    case (SiPMProperties::HitDistribution::kCircle):
      if (m_rng.Rand() < 0.95) { // In circle
        do {
          x = m_rng.Rand() * 2 - 1;      // x in [-1,1]
          y = m_rng.Rand() * 2 - 1;      // y in [-1,1]
        } while ((x * x) + (y * y) > 1); // if in unitary circle
        row = (x + 1) * m_Properties.nSideCells() / 2;
        col = (y + 1) * m_Properties.nSideCells() / 2;
      } else { // Outside
        do {
          x = m_rng.Rand() * 2 - 1;      // x in [-1,1]
          y = m_rng.Rand() * 2 - 1;      // y in [-1,1]
        } while ((x * x) + (y * y) < 1); // if outside in unitary circle
        row = (x + 1) * m_Properties.nSideCells() / 2;
        col = (y + 1) * m_Properties.nSideCells() / 2;
      }
      break;

    // Gaussian distribution centered in the sensor
    case (SiPMProperties::HitDistribution::kGaussian):
      x = m_rng.randGaussian(0, 1);
      y = m_rng.randGaussian(0, 1);

      if (abs(x) < 1.64 && abs(y) < 1.64) { // 95% of samples (1.64 sigmas)
        row = (x + 1.64) * (m_Properties.nSideCells() / 3.28);
        col = (y + 1.64) * (m_Properties.nSideCells() / 3.28);
      } else {
        row = m_rng.randInteger(nSideCells);
        col = m_rng.randInteger(nSideCells);
      }
      break;
  }
  return std::make_pair(row,col);
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
  const double pde = m_Properties.pde();
  m_Hits.reserve(nPhotons);

  switch (m_Properties.pdeType()) {
    // Add all photons
    case (SiPMProperties::PdeType::kNoPde):
      for (uint32_t i = 0; i < nPhotons; ++i) {

        std::pair<int32_t, int32_t> position = hitCell();
        m_Hits.emplace_back(m_PhotonTimes[i], 1, position.first, position.second, SiPMHit::HitType::kPhotoelectron);
        ++m_nTotalHits;
        ++m_nPe;
      }
      break;

    // Simple pde
    case (SiPMProperties::PdeType::kSimplePde):
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (isDetected(pde)) {

          std::pair<int32_t, int32_t> position = hitCell();
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

          std::pair<int32_t, int32_t> position = hitCell();
          m_Hits.emplace_back(m_PhotonTimes[i], 1, position.first, position.second, SiPMHit::HitType::kPhotoelectron);
          ++m_nTotalHits;
          ++m_nPe;
        }
      }
      break;
  } /* SWITCH */
}

void SiPMSensor::addXtEvents() {
  // To generate poisson process exp(-mu) is needed
  const double expxt = exp(-m_Properties.xt());
  const bool hasdxt = m_Properties.hasDXt();
  // Fraction of dt events that is dxt
  const double dxtprob = m_Properties.dxt();
  // Tau of delay of dxt
  const double dxttau = m_Properties.dxtTau();
  const double signalLength = m_Properties.signalLength();


  // Use while becouse number of hits increases in loop and iterator gets invalidated
  uint32_t currentCellIdx = 0;
  while (currentCellIdx < m_nTotalHits) {
    const SiPMHit currentHit = m_Hits[currentCellIdx];
    const double xtTime = currentHit.time();
    const int32_t xtGeneratorRow = currentHit.row();
    const int32_t xtGeneratorCol = currentHit.col();

    // Poisson process algorithm
    double test = m_rng.Rand();
    while (test > expxt) {
      // Decide if xt will be delayed with respect to generating cell
      const bool isdelayed = m_rng.Rand() < dxtprob;

      int32_t rowAdd, colAdd;
      // Do not add same cell
      do {
        rowAdd = m_rng.randInteger(2) - 1;  // [-1,0,1]
        colAdd = m_rng.randInteger(2) - 1;  // [-1,0,1]
      } while (rowAdd == 0 && colAdd == 0);
      const int32_t xtRow = xtGeneratorRow + rowAdd;
      const int32_t xtCol = xtGeneratorCol + colAdd;

      if (isInSensor(xtRow, xtCol)) {
        if (hasdxt && isdelayed){
          const double xtTimeDelayed = xtTime +  m_rng.randExponential(dxttau);
          if (xtTimeDelayed < signalLength){
            m_Hits.emplace_back(xtTimeDelayed, 1, xtRow, xtCol, SiPMHit::HitType::kDelayedOpticalCrosstalk, currentHit);
            ++m_nTotalHits;
            ++m_nXt;
            ++m_nDXt;
            ++m_nPe;
          }
        } else {
          m_Hits.emplace_back(xtTime, 1, xtRow, xtCol, SiPMHit::HitType::kOpticalCrosstalk, currentHit);
          ++m_nTotalHits;
          ++m_nXt;
          ++m_nPe;
        }
        m_Hits[currentCellIdx].addChildren(m_Hits.back());
      }
      test *= m_rng.Rand();
    } /* WHILE TEST < XT */
    currentCellIdx++;
  } /* WHILE HIT */
}

void SiPMSensor::addApEvents() {
  const double expap = exp(-m_Properties.ap());
  const double tauApFast = m_Properties.tauApFast();
  const double tauApSlow = m_Properties.tauApSlow();
  const double signalLength = m_Properties.signalLength();
  const double recoveryTime = m_Properties.recoveryTime();
  const double slowFraction = m_Properties.apSlowFraction();

  // Use while becouse number of hits increases in loop and iterator gets invalidated
  uint32_t currentCellIdx = 0;
  while (currentCellIdx < m_nTotalHits) {
    const SiPMHit currentHit = m_Hits[currentCellIdx];
    currentCellIdx++;
    const double apGeneratorTime = currentHit.time();
    // Poisson process algorithm (expap = exp(-ap))
    double test = m_rng.Rand();

    while (test > expap) {
      const bool isslow = m_rng.Rand() < slowFraction;
      double apDelay;
      if (isslow) {
        apDelay = m_rng.randExponential(tauApSlow);
      } else {
        apDelay = m_rng.randExponential(tauApFast);
      }

      // If ap event is in signal window
      if (apGeneratorTime + apDelay < signalLength) {
        const double apAmplitude = currentHit.amplitude() * (1 - exp(-apDelay / recoveryTime));

        // Ap event is delayed but in same cell
        if(isslow){
          m_Hits.emplace_back(apGeneratorTime + apDelay, apAmplitude, currentHit.row(), currentHit.col(),
          SiPMHit::HitType::kSlowAfterPulse, currentHit);
        } else {
          m_Hits.emplace_back(apGeneratorTime + apDelay, apAmplitude, currentHit.row(), currentHit.col(),
          SiPMHit::HitType::kFastAfterPulse, currentHit);

        }
        // Append ap hit to list of childrens for current ap generator
        m_Hits[currentCellIdx].addChildren(m_Hits.back());

        ++m_nTotalHits;
        ++m_nAp;
      }
      test *= m_rng.Rand();
    } /* WHILE TEST < AP */
  }   /* WHILE HIT */
}

void SiPMSensor::calculateSignalAmplitudes() {
  // Hits are sorted inplace such that thay have increasing times
  std::sort(m_Hits.begin(), m_Hits.end());
  const double tauRecovery = 1 / m_Properties.recoveryTime();

  for (auto hit = m_Hits.begin(); hit != m_Hits.end(); ++hit) {
    // Check if cell is fired more than once
    if (std::count(m_Hits.begin(), m_Hits.end(), *hit) > 1) {
      // If so check which hits are in same cell
      double previousHitTime = 0;
      for (auto test = m_Hits.begin(); test != m_Hits.end(); ++test) {
        if (*test == *hit) {
          // Branchless equivalent of if/else previousHitTime != 0
          // In first occurence prevHitTime is 0 so amplitude wille be 1.
          // Else if prevHitTime is not 0 the exp term will be considered
          double delay = hit->time() - previousHitTime;
          hit->amplitude() = 1 - exp(-delay * tauRecovery) * (int)(previousHitTime != 0);
          previousHitTime = hit->time();
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
  const double sampling = m_Properties.sampling();

  // Start with gaussian noise
  m_Signal = m_rng.randGaussian(0, m_Properties.snrLinear(), nSignalPoints);
  if (m_Hits.size() == 0) {
    return;
  }

  for (auto hit : m_Hits) {
    const int32_t time = hit->time() / sampling;
    const double amplitude = hit->amplitude() * m_rng.randGaussian(1, m_Properties.ccgv());

    const __m256d __amplitude = _mm256_set1_pd(amplitude);

    // Skipping tail of loop (no problem since it will be far from the signal (many taus)
    for (uint32_t i = time; i < nSignalPoints - 16; i += 16) {
      __m256d __signal1 = _mm256_loadu_pd(&m_Signal[i]);
      __m256d __signal2 = _mm256_loadu_pd(&m_Signal[i + 4]);
      __m256d __signal3 = _mm256_loadu_pd(&m_Signal[i + 8]);
      __m256d __signal4 = _mm256_loadu_pd(&m_Signal[i + 12]);
      __m256d __shape1 = _mm256_loadu_pd(&m_SignalShape[i - time]);
      __m256d __shape2 = _mm256_loadu_pd(&m_SignalShape[i - time + 4]);
      __m256d __shape3 = _mm256_loadu_pd(&m_SignalShape[i - time + 8]);
      __m256d __shape4 = _mm256_loadu_pd(&m_SignalShape[i - time + 12]);
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
  const double sampling = m_Properties.sampling();

  // Start with gaussian noise
  m_Signal = m_rng.randGaussian(0, m_Properties.snrLinear(), nSignalPoints);
  if (nHits == 0) {
    return;
  }

  // Temp storage in array for vectorization
  alignas(64) uint32_t times[nHits];
  alignas(64) double amplitudes[nHits];
  for (uint32_t i = 0; i < nHits; ++i) {
    times[i] = m_Hits[i].time() / sampling;
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

std::ostream& operator<<(std::ostream& os, SiPMSensor const& x) {
  os << "===> SiPM Sensor Start <===\n";
  os << x.properties() << "\n";
  os << x.debug();
  return os;
}

void SiPMSensor::dumpHits() {
  std::cout << std::string(65, '=') << "\n";
  std::cout << printCenter("Hit Time", 15) << " | " << printCenter("Hit Amplitude", 15) << " | "
            << printCenter("Hit Position", 15) << " | " << printCenter("Hit Type", 15) << "\n";
  for (auto&& h : m_Hits) {
    std::cout << printDouble(h.time(), 4, 15) << " | " << printDouble(h.amplitude(), 4, 15) << " | "
              << printCenter(std::to_string(h.row()) + " - " + std::to_string(h.col()), 15) << " | ";
    switch (h.hitType()) {
      case SiPMHit::HitType::kPhotoelectron:
        std::cout << printCenter("Pe", 15) << "\n";
        break;
      case SiPMHit::HitType::kDarkCount:
        std::cout << printCenter("Dcr", 15) << "\n";
        break;
      case SiPMHit::HitType::kOpticalCrosstalk:
        std::cout << printCenter("Xt", 15) << "\n";
        break;
      case SiPMHit::HitType::kDelayedOpticalCrosstalk:
        std::cout << printCenter("DXt", 15) << "\n";
        break;
      case SiPMHit::HitType::kFastAfterPulse:
        std::cout << printCenter("Ap (fast)", 15) << "\n";
        break;
      case SiPMHit::HitType::kSlowAfterPulse:
        std::cout << printCenter("Ap (slow)", 15) << "\n";
        break;
    }
  }
}
} // namespace sipm
