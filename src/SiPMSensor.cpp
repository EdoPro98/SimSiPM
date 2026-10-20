#include "SiPMSensor.h"
#include "SiPMAnalogSignal.h"
#include "SiPMHit.h"
#include "SiPMProperties.h"
#include "SiPMRandom.h"
#include "SiPMTypes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <ostream>
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
  m_PhotonTimes.insert(m_PhotonTimes.end(), val.begin(), val.end());
}

void SiPMSensor::addPhotons(const std::vector<double>& val1, const std::vector<double>& val2) {
  const size_t n = std::min(val1.size(), val2.size());
  if (val1.size() != val2.size()) {
    std::cerr << "addPhotons: times and wavelengths have different sizes (" << val1.size() << " vs " << val2.size()
               << "), extra entries are ignored" << std::endl;
  }
  m_PhotonTimes.insert(m_PhotonTimes.end(), val1.begin(), val1.begin() + n);
  m_PhotonWavelengths.insert(m_PhotonWavelengths.end(), val2.begin(), val2.begin() + n);
}

void SiPMSensor::runEvent() {
  m_Signal = SiPMAnalogSignal(
    m_rng.randGaussianF(0.0, m_Properties.snrLinear(), m_Properties.nSignalPoints()),
    m_Properties.sampling()
  );
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
  m_Hits.clear();
  m_PhotonTimes.clear();
  m_PhotonWavelengths.clear();
}

void SiPMSensor::setSignalShape(const std::vector<float>& shape, double sampling) {
  m_Properties.setCustomSignalShape(sampling, static_cast<uint32_t>(shape.size()));
  m_SignalShape = shape;
}

void SiPMSensor::setSignalShape(const std::vector<double>& shape, double sampling) {
  m_Properties.setCustomSignalShape(sampling, static_cast<uint32_t>(shape.size()));
  m_SignalShape.resize(shape.size());
  for (size_t i = 0; i < shape.size(); ++i) {
    m_SignalShape[i] = static_cast<float>(shape[i]);
  }
}

void SiPMSensor::signalShape() {
  if (m_Properties.hasCustomSignalShape()) return;

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
  if (pde.size() < 2) { return 0; }
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
  if (m_rng.Rand() <= 0.90) { // In circle
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

  // Starting generation "before" the signal window gives better results.
  // Advance past t=0 without recording, then record every subsequent hit.
  double last = -3 * meanDcr;
  while (last <= 0) {
    last += m_rng.randExponential(meanDcr);
  }
  while (last < signalLength) {
    const pair<uint32_t> rowcol = m_rng.randInteger2(nSideCells);
    m_Hits.emplace_back(last, 1.f, rowcol.first, rowcol.second, SiPMHit::HitType::kDarkCount);
    ++m_nTotalHits;
    ++m_nDcr;
    ++m_nPe;
    last += m_rng.randExponential(meanDcr);
  }
}

void SiPMSensor::addPhotoelectrons() {
  const double sigLen = m_Properties.signalLength();
  const uint32_t nPhotons = m_PhotonTimes.size();
  const SiPMProperties::PdeType pdeType = m_Properties.pdeType();
  constexpr SiPMHit::HitType photoelectron = SiPMHit::HitType::kPhotoelectron;
  m_Hits.reserve(m_Hits.size() + nPhotons);

  switch (pdeType) {
    case SiPMProperties::PdeType::kNoPde:
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (m_PhotonTimes[i] < 0 || m_PhotonTimes[i] >= sigLen) { continue; }
        const pair<uint32_t> position = hitCell();
        m_Hits.emplace_back(m_PhotonTimes[i], 1.f, position.first, position.second, photoelectron);
        m_nTotalHits++;
        m_nPe++;
      }
      return;
    case SiPMProperties::PdeType::kSimplePde:
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (m_PhotonTimes[i] < 0 || m_PhotonTimes[i] >= sigLen) { continue; }
        if (m_rng.Rand() <= m_Properties.pde()) {
          const pair<uint32_t> position = hitCell();
          m_Hits.emplace_back(m_PhotonTimes[i], 1.f, position.first, position.second, photoelectron);
          m_nTotalHits++;
          m_nPe++;
        }
      }
      return;
    case SiPMProperties::PdeType::kSpectrumPde:
      for (uint32_t i = 0; i < nPhotons; ++i) {
        if (m_PhotonTimes[i] < 0 || m_PhotonTimes[i] >= sigLen) { continue; }
        if (i < m_PhotonWavelengths.size() && m_rng.Rand() <= evaluatePde(m_PhotonWavelengths[i])) {
          const pair<uint32_t> position = hitCell();
          m_Hits.emplace_back(m_PhotonTimes[i], 1.f, position.first, position.second, photoelectron);
          m_nTotalHits++;
          m_nPe++;
        }
      }
      return;
  }
}

SiPMHit SiPMSensor::generateXtHit(const SiPMHit& xtGen, int32_t parentIdx) const {
  int32_t xtRow, xtCol;
  const int32_t row = xtGen.row();
  const int32_t col = xtGen.col();
  const bool isDelayed = m_Properties.hasDXt() && (m_rng.Rand() <= m_Properties.dxt());
  const SiPMHit::HitType hitType = isDelayed ? SiPMHit::HitType::kDelayedOpticalCrosstalk : SiPMHit::HitType::kOpticalCrosstalk;

  do {
    xtRow = row + m_rng.randInteger(3) - 1;
    xtCol = col + m_rng.randInteger(3) - 1;
  } while (((xtRow == row) && (xtCol == col)) || !isInSensor(xtRow, xtCol));

  double xtDelay = 0;
  if (isDelayed) {
    // Strict '>=' keeps every hit time strictly below signalLength, matching
    // addDcrEvents/addPhotoelectrons, so generateSignal() can trust time < nSignalPoints.
    do {
      xtDelay = m_rng.randExponential(m_Properties.dxtTau());
    } while (xtGen.time() + xtDelay >= m_Properties.signalLength());
  }
  return SiPMHit{xtGen.time() + xtDelay, 1.f, (uint32_t)xtRow, (uint32_t)xtCol, hitType, parentIdx};
}

SiPMHit SiPMSensor::generateApHit(const SiPMHit& apGen, int32_t parentIdx) const {
  const bool isSlow = m_rng.Rand() <= m_Properties.apSlowFraction();
  const SiPMHit::HitType hitType = isSlow ? SiPMHit::HitType::kSlowAfterPulse : SiPMHit::HitType::kFastAfterPulse;
  const float tau = isSlow ? m_Properties.tauApSlow() : m_Properties.tauApFast();
  double delay;
  // Strict '>=' keeps every hit time strictly below signalLength, matching
  // addDcrEvents/addPhotoelectrons, so generateSignal() can trust time < nSignalPoints.
  do {
    delay = m_rng.randExponential(tau);
  } while (apGen.time() + delay >= m_Properties.signalLength());

  return SiPMHit{apGen.time() + delay, 1.f, apGen.row(), apGen.col(), hitType, parentIdx};
}

void SiPMSensor::addCorrelatedNoise() {
  const bool hasXt = m_Properties.hasXt();
  const bool hasAp = m_Properties.hasAp();

  const double xtExpMu = exp(-m_Properties.xt());
  const double apExpMu = exp(-m_Properties.ap());

  for (uint32_t currentHitIdx = 0; currentHitIdx < m_nTotalHits; ++currentHitIdx) {
    double xtPoiss = m_rng.Rand() * (int)(hasXt);
    double apPoiss = m_rng.Rand() * (int)(hasAp);

    // XT — copy parent data before push_back (which may reallocate m_Hits)
    while (xtPoiss > xtExpMu) {
      SiPMHit xtHit = generateXtHit(m_Hits[currentHitIdx], (int32_t)currentHitIdx);
      m_nDXt += (int)(xtHit.hitType() == SiPMHit::HitType::kDelayedOpticalCrosstalk);
      m_Hits.push_back(std::move(xtHit));
      m_nTotalHits++;
      m_nXt++;
      m_nPe++;
      xtPoiss *= m_rng.Rand();
    }

    // AP
    while (apPoiss > apExpMu) {
      SiPMHit apHit = generateApHit(m_Hits[currentHitIdx], (int32_t)currentHitIdx);
      m_Hits.push_back(std::move(apHit));
      m_nTotalHits++;
      m_nAp++;
      m_nPe++;
      apPoiss *= m_rng.Rand();
    }
  }
}

void SiPMSensor::calculateSignalAmplitudes() {
  const double recoveryRate = 1 / m_Properties.recoveryTime();
  const uint32_t n = m_nTotalHits;
  const uint32_t nSideCells = m_Properties.nSideCells();
  const float ccgv = m_Properties.ccgv();

  if (n == 1) {
    m_Hits[0].amplitude() *= m_rng.randGaussianF(1.0f, ccgv);
    return;
  }

  const auto cellKey = [nSideCells](const SiPMHit& h) {
    return h.col() + nSideCells * h.row();
  };
  const auto hitCmp = [&cellKey](const SiPMHit& a, const SiPMHit& b) {
    const uint32_t ka = cellKey(a), kb = cellKey(b);
    return ka != kb ? ka < kb : a.time() < b.time();
  };

  if (n < 16) {
    for (uint32_t i = 0; i < n; ++i)
      m_Hits[i].amplitude() *= m_rng.randGaussianF(1.0f, ccgv);

    if (n <= 1) return;

    bool anyCollision = false;
    for (uint32_t i = 0; i < n && !anyCollision; ++i)
      for (uint32_t j = i + 1; j < n; ++j)
        if (m_Hits[i].row() == m_Hits[j].row() && m_Hits[i].col() == m_Hits[j].col()) {
          anyCollision = true; break;
        }
    if (!anyCollision) return;

    // Sorting reorders m_Hits, which would invalidate parentIdx (an index into
    // m_Hits) for every XT/AP hit whose parent moves. Precompute each hit's
    // final position with a tiny sort over plain indices (n < 16, stack-only,
    // no SiPMHit copies) and rewrite parentIdx in place before physically
    // sorting: std::sort's sequence of comparisons for a given n depends only
    // on the comparator's results, not on the element type being moved, so
    // sorting indices with the same comparator predicts the exact order the
    // real sort below will produce.
    std::array<uint32_t, 16> order;
    for (uint32_t i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.begin() + n,
              [&](uint32_t a, uint32_t b) { return hitCmp(m_Hits[a], m_Hits[b]); });

    std::array<uint32_t, 16> newPosOf;
    for (uint32_t k = 0; k < n; ++k) newPosOf[order[k]] = k;

    for (uint32_t i = 0; i < n; ++i) {
      int32_t& parent = m_Hits[i].parentIdx();
      if (parent >= 0) parent = static_cast<int32_t>(newPosOf[parent]);
    }

    std::sort(m_Hits.begin(), m_Hits.end(), hitCmp);
  } else {
    // Large-N path: proxy array keeps key+time+idx contiguous so sort
    // comparisons stay cache-friendly without chasing per-element pointers.
    const std::vector<float> ccgvs = m_rng.randGaussianF(1.0f, ccgv, n);
    for (uint32_t i = 0; i < n; ++i)
      m_Hits[i].amplitude() *= ccgvs[i];

    struct Proxy { uint32_t key; uint32_t idx; double time; };
    std::vector<Proxy> proxies(n);
    for (uint32_t i = 0; i < n; ++i)
      proxies[i] = { cellKey(m_Hits[i]), i, m_Hits[i].time() };
    std::sort(proxies.begin(), proxies.end(), [](const Proxy& a, const Proxy& b) {
      return a.key != b.key ? a.key < b.key : a.time < b.time;
    });

    uint32_t prevKey = proxies[0].key;
    for (uint32_t i = 1; i < n; ++i) {
      if (proxies[i].key == prevKey) {
        const double delay = proxies[i].time - proxies[i - 1].time;
        m_Hits[proxies[i].idx].amplitude() *= 1 - exp(-delay * recoveryRate);
      }
      prevKey = proxies[i].key;
    }
    return;
  }

  // Recovery scan for the small-N path (after sort)
  uint32_t prevKey = cellKey(m_Hits[0]);
  for (uint32_t i = 1; i < n; ++i) {
    const uint32_t k = cellKey(m_Hits[i]);
    if (k == prevKey) {
      const double delay = m_Hits[i].time() - m_Hits[i - 1].time();
      m_Hits[i].amplitude() *= 1 - exp(-delay * recoveryRate);
    }
    prevKey = k;
  }
}


void SiPMSensor::generateSignal() {
  const uint32_t nSignalPoints = m_Properties.nSignalPoints();
  const double sampling = m_Properties.sampling();

  std::vector<uint32_t> times(m_nTotalHits);
  std::vector<float> amplitudes(m_nTotalHits);
  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    times[i] = static_cast<uint32_t>(std::floor(m_Hits[i].time() / sampling));
    amplitudes[i] = m_Hits[i].amplitude();
  }

  for (uint32_t i = 0; i < m_nTotalHits; ++i) {
    const uint32_t time = times[i];
    const uint32_t endPoint = nSignalPoints - time;
    const float amplitude = amplitudes[i];

    float* __restrict__       signalPtr      = m_Signal.data() + time;
    const float* __restrict__ signalShapePtr = m_SignalShape.data();

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
