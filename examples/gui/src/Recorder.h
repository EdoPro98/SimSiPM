#ifndef SIPM_GUI_RECORDER_H
#define SIPM_GUI_RECORDER_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "BoundedQueue.h"

namespace sipmgui {

/// Compressed block of waveforms, produced by the workers.
struct WaveBlock {
  uint32_t nEvents = 0;
  uint32_t rawSize = 0;            // bytes of the encoded stream before deflate
  uint64_t samples = 0;            // samples in the block (statistics)
  std::vector<unsigned char> data; // deflated stream
};

/// Encodes waveforms into a block (run by the workers, in parallel).
///
/// Per waveform: varint id, varint nSources, varint nSamples, then the
/// samples quantised as q = round(v / lsb), stored as zigzag varints of
/// q[0] and of the differences q[i] - q[i-1]. The block is then compressed
/// with zlib (deflate, fastest level): noise-like differences mostly take
/// one byte and deflate removes most of what is left.
class BlockEncoder {
public:
  void begin() {
    m_Raw.clear();
    m_Events = 0;
    m_Samples = 0;
  }
  void add(uint64_t id, uint32_t nSources, const float* samples, size_t n, double lsb);
  bool empty() const { return m_Events == 0; }
  WaveBlock finish(int level);

private:
  void varint(uint64_t v);
  std::vector<unsigned char> m_Raw;
  uint32_t m_Events = 0;
  uint64_t m_Samples = 0;
};

/// Writes waveform blocks to a file from a dedicated thread.
///
/// File layout (little endian):
///   "SIPMWAV1"                          8 bytes magic
///   uint32 headerSize, header           JSON: settings, run info, encoding
///   blocks: uint32 nEvents, uint32 rawSize, uint32 dataSize, data[dataSize]
///   end:    uint32 0, uint32 summarySize, summary (JSON: totals)
/// Blocks are written in completion order: sort by event id if needed.
class Recorder {
public:
  struct Stats {
    bool active = false; // accepting events or still writing
    uint64_t events = 0; // waveforms written
    uint64_t target = 0; // 0 = until stopped
    uint64_t fileBytes = 0;
    uint64_t floatBytes = 0; // size the samples would have as float32
    double seconds = 0;
    std::string path;
  };

  ~Recorder() { stop(); }

  /// Opens the file and writes the header. maxEvents = 0 records until stop().
  std::string start(const std::string& path, const std::string& headerJson, double lsb, uint64_t maxEvents);
  /// Stops accepting events, writes the remaining blocks and closes the file
  void stop();

  bool accepting() const { return m_Accepting.load(); }
  double lsb() const { return m_Lsb; }
  /// Workers: reserve up to n waveforms; the returned count must be encoded and submitted
  uint32_t reserve(uint32_t n);
  /// Workers: hands over a block of reserved waveforms (blocks if the writer is behind)
  void submit(WaveBlock&& block);
  /// True once the requested number of waveforms has been written
  bool finished() const { return m_Done.load(); }
  Stats stats() const;

private:
  void writerLoop();
  void finalize();

  BoundedQueue<WaveBlock> m_Queue{256};
  std::thread m_Writer;
  FILE* m_File = nullptr;
  std::string m_Path;
  double m_Lsb = 1e-3;
  uint64_t m_Target = 0;
  std::atomic<bool> m_Accepting{false};
  std::atomic<bool> m_Done{false};
  std::atomic<uint64_t> m_Reserved{0};
  std::atomic<int64_t> m_InFlight{0};
  std::atomic<uint64_t> m_Written{0}, m_FileBytes{0}, m_FloatBytes{0};
  std::chrono::steady_clock::time_point m_Start;
  mutable std::mutex m_Mutex;
  double m_Seconds = 0;
};

} // namespace sipmgui
#endif
