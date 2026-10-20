#include "Pipeline.h"

#include <algorithm>
#include <cmath>

#include <zlib.h>

#include "SiPM.h"

namespace sipmgui {
namespace {

constexpr size_t kInQueueBatches = 1024;
constexpr size_t kOutQueueBatches = 1024;

/// Translates the GUI configuration into library properties. The simulated
/// signal is longer than the window by the history margin.
sipm::SiPMProperties makeProperties(const SimConfig& c, double margin) {
  using sipm::SiPMProperties;
  SiPMProperties p;
  p.setSize(c.sizeMm);
  p.setPitch(c.pitchUm);
  p.setHitDistribution(static_cast<SiPMProperties::HitDistribution>(c.lightProfile));

  p.setSampling(c.sampling);
  p.setSignalLength(c.signalLength + margin);
  p.setRiseTime(c.riseTime);
  p.setFallTimeFast(c.fallTimeFast);
  p.setFallTimeSlow(c.fallTimeSlow);
  p.setSlowComponentFraction(c.slowPct / 100.0);
  if (!c.slowOn) {
    p.setSlowComponentOff();
  }
  p.setRecoveryTime(c.recoveryTime);

  p.setDcr(c.dcrKHz * 1e3);
  if (!c.dcrOn || c.dcrKHz <= 0) {
    p.setDcrOff();
  }
  p.setXt(c.xtPct / 100.0);
  if (!c.xtOn) {
    p.setXtOff();
  }
  p.setDXt(c.dxtPct / 100.0);
  p.setDXtTau(c.dxtTau);
  if (!c.dxtOn || !c.xtOn) {
    p.setDXtOff();
  }
  p.setAp(c.apPct / 100.0);
  p.setTauApFastComponent(c.apTauFast);
  p.setTauApSlowComponent(c.apTauSlow);
  p.setApSlowFraction(c.apSlowPct / 100.0);
  if (!c.apOn) {
    p.setApOff();
  }
  p.setCcgv(c.ccgvPct / 100.0);
  p.setSnr(c.snrDb);

  switch (c.pdeMode) {
    case PdeMode::kNone:
      p.setPdeType(SiPMProperties::PdeType::kNoPde);
      break;
    case PdeMode::kConstant:
      p.setPde(c.pdePct / 100.0);
      break;
    case PdeMode::kSpectrum:
      if (c.pdeWavelengths.size() >= 2) {
        p.setPdeSpectrum(c.pdeWavelengths, c.pdeValues);
      } else {
        p.setPdeType(SiPMProperties::PdeType::kNoPde);
      }
      break;
  }
  return p;
}

} // namespace

Pipeline::Pipeline() : m_InQueue(kInQueueBatches), m_OutQueue(kOutQueueBatches) {
  m_Config = std::make_shared<const SimConfig>();
}

Pipeline::~Pipeline() { stop(); }

void Pipeline::start(int nWorkers) {
  stop();
  m_Stop = false;
  m_InQueue.reopen();
  m_OutQueue.reopen();
  m_NWorkers = std::max(1, nWorkers);
  m_Threads.emplace_back(&Pipeline::generatorLoop, this);
  for (int i = 0; i < m_NWorkers; ++i) {
    m_Threads.emplace_back(&Pipeline::workerLoop, this);
  }
}

void Pipeline::stop() {
  if (m_Threads.empty()) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(m_RunMutex);
    m_Stop = true;
  }
  m_RunCv.notify_all();
  m_InQueue.close();
  m_OutQueue.close();
  for (auto& t : m_Threads) {
    t.join();
  }
  m_Threads.clear();
}

void Pipeline::setRunning(bool run) {
  {
    std::lock_guard<std::mutex> lock(m_RunMutex);
    m_Running = run;
  }
  m_RunCv.notify_all();
}

uint64_t Pipeline::setConfig(const SimConfig& cfg) {
  auto snapshot = std::make_shared<const SimConfig>(cfg);
  uint64_t v;
  {
    std::lock_guard<std::mutex> lock(m_ConfigMutex);
    m_Config = std::move(snapshot);
    v = ++m_Version;
  }
  // Stale work would be discarded anyway, dropping it here frees the FIFOs sooner
  m_InQueue.clear();
  m_OutQueue.clear();
  return v;
}

std::shared_ptr<const SimConfig> Pipeline::currentConfig(uint64_t& version) const {
  std::lock_guard<std::mutex> lock(m_ConfigMutex);
  version = m_Version.load();
  return m_Config;
}

void Pipeline::generatorLoop() {
  sipm::SiPMRandom rng;
  uint64_t version = ~0ull;
  std::shared_ptr<const SimConfig> cfg;
  std::unique_ptr<EventStream> stream;
  uint64_t nextId = 0;

  while (!m_Stop) {
    if (!m_Running) {
      std::unique_lock<std::mutex> lock(m_RunMutex);
      m_RunCv.wait(lock, [this] { return m_Stop || m_Running; });
      continue;
    }
    if (m_Version.load() != version) {
      cfg = currentConfig(version);
      stream = std::make_unique<EventStream>(*cfg);
    }

    EventBatch batch;
    batch.config = cfg;
    batch.version = version;
    batch.firstId = nextId;
    batch.margin = stream->margin();
    batch.events.resize(kBatchSize);
    for (auto& ev : batch.events) {
      stream->next(ev, rng);
    }
    nextId += kBatchSize;

    if (!m_InQueue.push(std::move(batch))) {
      break;
    }
    nGenerated += kBatchSize;
  }
}

/// Copies waveform and hits of a window (hits shifted to window time)
static void takeSnapshot(EventResult& r, uint64_t id, const EventFeatures& f, const PhotonEvent& ev,
                         const sipm::SiPMAnalogSignal& signal, const std::vector<sipm::SiPMHit>& hits, double margin) {
  r.id = id;
  r.f = f;
  r.nSources = ev.nSources;
  r.nPhotons =
    static_cast<uint32_t>(std::count_if(ev.times.begin(), ev.times.end(), [&](double t) { return t >= margin; }));
  r.waveform.assign(signal.waveform().begin(), signal.waveform().end());
  r.hits.clear();
  r.nByType.fill(0);
  for (const auto& h : hits) {
    if (h.time() < margin) {
      continue; // history before the window
    }
    const auto type = static_cast<uint8_t>(h.hitType());
    r.hits.push_back(HitLite{static_cast<float>(h.time() - margin), h.row(), h.col(), type});
    if (type < r.nByType.size()) {
      ++r.nByType[type];
    }
  }
}

void Pipeline::workerLoop() {
  sipm::SiPMSensor sensor;
  uint64_t sensorVersion = ~0ull;
  EventBatch batch;
  std::vector<float> window;
  BlockEncoder encoder;

  while (m_InQueue.pop(batch)) {
    if (batch.version != m_Version.load()) {
      continue; // stale
    }
    // While recording every event is simulated and stored, whatever the GUI does
    const uint32_t nRecord = m_Recorder.reserve(static_cast<uint32_t>(batch.events.size()));
    if (nRecord == 0 && m_OutQueue.full()) {
      // The GUI is behind: drop now instead of simulating events nobody will see
      nDropped += batch.events.size();
      continue;
    }
    encoder.begin();
    const SimConfig& cfg = *batch.config;
    if (batch.version != sensorVersion) {
      sensor.setProperties(makeProperties(cfg, batch.margin));
      sensorVersion = batch.version;
    }
    const bool withWavelengths = cfg.pdeMode == PdeMode::kSpectrum;
    const float trigLevel = m_TriggerLevel.load();
    const auto skip = static_cast<size_t>(std::lround(batch.margin / cfg.sampling));

    ResultBatch out;
    out.version = batch.version;
    out.sampling = cfg.sampling;
    out.nSideCells = sensor.properties().nSideCells();
    out.features.reserve(batch.events.size());

    for (size_t i = 0; i < batch.events.size(); ++i) {
      const PhotonEvent& ev = batch.events[i];

      sensor.resetState();
      if (withWavelengths) {
        sensor.addPhotons(ev.times, ev.wavelengths);
      } else {
        sensor.addPhotons(ev.times);
      }
      sensor.runEvent();

      // Drop the history margin: features and display only see the window
      const sipm::SiPMAnalogSignal full = sensor.signal();
      const auto& w = full.waveform();
      window.assign(w.begin() + std::min(skip, w.size()), w.end());
      const sipm::SiPMAnalogSignal signal(window, cfg.sampling);

      EventFeatures f;
      f.peak = signal.peak(cfg.intStart, cfg.intWindow, cfg.threshold);
      f.integral = signal.integral(cfg.intStart, cfg.intWindow, cfg.threshold);
      f.tot = signal.tot(cfg.intStart, cfg.intWindow, cfg.threshold);
      f.toa = signal.toa(cfg.intStart, cfg.intWindow, cfg.threshold);
      f.maxAmp = window.empty() ? 0.f : *std::max_element(window.begin(), window.end());
      out.features.push_back(f);

      const uint64_t id = batch.firstId + i;
      if (i < nRecord) {
        encoder.add(id, ev.nSources, window.data(), window.size(), m_Recorder.lsb());
      }
      const bool triggered = f.maxAmp > trigLevel;
      const bool isLast = i + 1 == batch.events.size();
      if (isLast) {
        takeSnapshot(out.last, id, f, ev, signal, sensor.hits(), batch.margin);
        out.lastIsTriggered = triggered;
      } else if (triggered) {
        takeSnapshot(out.lastTriggered, id, f, ev, signal, sensor.hits(), batch.margin);
      }
      out.hasTriggered |= triggered;
    }

    if (nRecord > 0) {
      m_Recorder.submit(encoder.finish(Z_BEST_SPEED));
    }
    nSimulated += batch.events.size();
    if (!m_OutQueue.tryPush(std::move(out))) {
      nDropped += batch.events.size();
    }
  }
}

} // namespace sipmgui
