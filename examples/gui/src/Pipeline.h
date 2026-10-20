#ifndef SIPM_GUI_PIPELINE_H
#define SIPM_GUI_PIPELINE_H

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "BoundedQueue.h"
#include "Config.h"
#include "Recorder.h"
#include "Source.h"

namespace sipmgui {

/// Unit of work in the input FIFO. Events are batched to keep queue
/// locking negligible compared to the simulation itself.
struct EventBatch {
  std::shared_ptr<const SimConfig> config;
  uint64_t version = 0;
  uint64_t firstId = 0;
  double margin = 0; // history simulated before each window [ns], cropped by the workers
  std::vector<PhotonEvent> events;
};

/// Compact copy of a SiPMHit, enough to draw the cell matrix.
struct HitLite {
  float time;
  uint32_t row;
  uint32_t col;
  uint8_t type; // SiPMHit::HitType
};

/// Scalar features of one event, sent to the GUI for every simulated event.
struct EventFeatures {
  float peak = -1;
  float integral = -1;
  float tot = -1;
  float toa = -1;
  float maxAmp = 0; // maximum over the full waveform, used for triggering
};

/// Full description of one event (waveform and hits), only sent for the
/// events the oscilloscope may display.
struct EventResult {
  uint64_t id = 0;
  EventFeatures f;
  uint32_t nPhotons = 0;             // photons inside the window
  uint32_t nSources = 0;             // source events with light in the window (pile-up if > 1)
  std::array<uint32_t, 6> nByType{}; // hit count indexed by SiPMHit::HitType
  std::vector<float> waveform;
  std::vector<HitLite> hits;
};

/// Output of a worker for one input batch. The scope only ever shows the
/// newest triggered event or the newest event, so only those two carry the
/// waveform; histograms are filled from the features of every event.
struct ResultBatch {
  uint64_t version = 0;
  double sampling = 1;
  uint32_t nSideCells = 0;
  std::vector<EventFeatures> features;
  EventResult last;             // newest event of the batch
  EventResult lastTriggered;    // newest event above the trigger level
  bool hasTriggered = false;    // lastTriggered is valid (or is `last`)
  bool lastIsTriggered = false; // the newest event is also the newest triggered one
};

/// Generator -> FIFO -> N SiPM workers -> FIFO -> GUI.
///
/// The generator blocks when the input FIFO is full. Workers never block on
/// the output FIFO: if it is full (the GUI does not keep up) the batch is
/// dropped before being simulated, so no work is spent on discarded events.
/// Every config change bumps a version number: work produced with an older
/// version is discarded wherever it is found.
class Pipeline {
public:
  static constexpr uint32_t kBatchSize = 32;

  Pipeline();
  ~Pipeline();

  void start(int nWorkers);
  void stop();
  bool isStarted() const { return !m_Threads.empty(); }
  int nWorkers() const { return m_NWorkers; }

  void setRunning(bool run);
  bool isRunning() const { return m_Running.load(); }

  /// Publishes a new configuration. Returns the new version number.
  uint64_t setConfig(const SimConfig& cfg);
  uint64_t version() const { return m_Version.load(); }

  /// Level used by the workers to pick the triggered event of each batch
  void setTriggerLevel(double level) { m_TriggerLevel = static_cast<float>(level); }

  BoundedQueue<ResultBatch>& results() { return m_OutQueue; }
  /// Waveform dump: fed directly by the workers, independent of the GUI
  Recorder& recorder() { return m_Recorder; }
  size_t inQueueSize() const { return m_InQueue.size(); }
  size_t inQueueCapacity() const { return m_InQueue.capacity(); }

  // Counters (monotonic, read by the GUI to compute rates)
  std::atomic<uint64_t> nGenerated{0};
  std::atomic<uint64_t> nSimulated{0};
  std::atomic<uint64_t> nDropped{0};

private:
  void generatorLoop();
  void workerLoop();

  std::shared_ptr<const SimConfig> currentConfig(uint64_t& version) const;

  Recorder m_Recorder;
  BoundedQueue<EventBatch> m_InQueue;
  BoundedQueue<ResultBatch> m_OutQueue;

  mutable std::mutex m_ConfigMutex;
  std::shared_ptr<const SimConfig> m_Config;
  std::atomic<uint64_t> m_Version{0};
  std::atomic<float> m_TriggerLevel{0};

  std::mutex m_RunMutex;
  std::condition_variable m_RunCv;
  std::atomic<bool> m_Running{true};
  std::atomic<bool> m_Stop{false};

  std::vector<std::thread> m_Threads;
  int m_NWorkers = 0;
};

} // namespace sipmgui
#endif
