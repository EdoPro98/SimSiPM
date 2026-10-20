#include "Recorder.h"

#include <cmath>

#include <zlib.h>

namespace sipmgui {
namespace {

void put32(FILE* f, uint32_t v) {
  const unsigned char b[4] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8),
                              static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 24)};
  std::fwrite(b, 1, 4, f);
}

constexpr uint32_t kEndOfStream = 0xFFFFFFFFu; // internal sentinel block

} // namespace

// ============================================================ BlockEncoder

void BlockEncoder::varint(uint64_t v) {
  while (v >= 0x80) {
    m_Raw.push_back(static_cast<unsigned char>(v | 0x80));
    v >>= 7;
  }
  m_Raw.push_back(static_cast<unsigned char>(v));
}

void BlockEncoder::add(uint64_t id, uint32_t nSources, const float* samples, size_t n, double lsb) {
  varint(id);
  varint(nSources);
  varint(n);
  m_Samples += n;
  const double inv = 1.0 / lsb;
  int64_t prev = 0;
  for (size_t i = 0; i < n; ++i) {
    const auto q = static_cast<int64_t>(std::llround(samples[i] * inv));
    const int64_t d = q - prev;
    prev = q;
    varint((static_cast<uint64_t>(d) << 1) ^ static_cast<uint64_t>(d >> 63)); // zigzag
  }
  ++m_Events;
}

WaveBlock BlockEncoder::finish(int level) {
  WaveBlock b;
  b.nEvents = m_Events;
  b.samples = m_Samples;
  b.rawSize = static_cast<uint32_t>(m_Raw.size());
  uLongf size = compressBound(m_Raw.size());
  b.data.resize(size);
  if (compress2(b.data.data(), &size, m_Raw.data(), m_Raw.size(), level) != Z_OK) {
    b.data.clear();
    b.nEvents = 0;
  }
  b.data.resize(size);
  return b;
}

// ================================================================ Recorder

std::string Recorder::start(const std::string& path, const std::string& headerJson, double lsb, uint64_t maxEvents) {
  stop();
  m_File = std::fopen(path.c_str(), "wb");
  if (m_File == nullptr) {
    return "Cannot write " + path;
  }
  std::fwrite("SIPMWAV1", 1, 8, m_File);
  put32(m_File, static_cast<uint32_t>(headerJson.size()));
  std::fwrite(headerJson.data(), 1, headerJson.size(), m_File);
  m_Path = path;
  m_Lsb = lsb;
  m_Target = maxEvents;
  m_Reserved = 0;
  m_InFlight = 0;
  m_Written = 0;
  m_FileBytes = 12 + headerJson.size();
  m_FloatBytes = 0;
  m_Done = false;
  m_Start = std::chrono::steady_clock::now();
  m_Queue.reopen();
  m_Writer = std::thread(&Recorder::writerLoop, this);
  m_Accepting = true;
  return {};
}

uint32_t Recorder::reserve(uint32_t n) {
  // Register first, then check: stop() clears m_Accepting before waiting for
  // m_InFlight to drop to zero, so a reservation is never lost
  ++m_InFlight;
  if (!m_Accepting) {
    --m_InFlight;
    return 0;
  }
  uint64_t got = n;
  if (m_Target > 0) {
    uint64_t cur = m_Reserved.load();
    do {
      got = std::min<uint64_t>(n, m_Target - std::min(cur, m_Target));
    } while (!m_Reserved.compare_exchange_weak(cur, cur + got));
    if (cur + got >= m_Target) {
      m_Accepting = false; // all requested waveforms are assigned
    }
  }
  if (got == 0) {
    --m_InFlight;
  }
  return static_cast<uint32_t>(got);
}

void Recorder::submit(WaveBlock&& block) {
  m_Queue.push(std::move(block));
  --m_InFlight;
}

void Recorder::writerLoop() {
  WaveBlock b;
  while (m_Queue.pop(b)) {
    if (b.nEvents == kEndOfStream) {
      break;
    }
    if (b.nEvents == 0) {
      continue;
    }
    put32(m_File, b.nEvents);
    put32(m_File, b.rawSize);
    put32(m_File, static_cast<uint32_t>(b.data.size()));
    std::fwrite(b.data.data(), 1, b.data.size(), m_File);
    m_FileBytes += 12 + b.data.size();
    m_Written += b.nEvents;
    m_FloatBytes += b.samples * sizeof(float);
    if (m_Target > 0 && m_Written >= m_Target) {
      break;
    }
  }
  finalize();
}

void Recorder::finalize() {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_Start).count();
  }
  char summary[160];
  const int n = std::snprintf(summary, sizeof(summary), "{\"events\": %llu, \"seconds\": %.3f}",
                              static_cast<unsigned long long>(m_Written.load()), m_Seconds);
  put32(m_File, 0);
  put32(m_File, static_cast<uint32_t>(n));
  std::fwrite(summary, 1, n, m_File);
  m_FileBytes += 8 + n;
  std::fclose(m_File);
  m_File = nullptr;
  m_Done = true;
}

void Recorder::stop() {
  if (!m_Writer.joinable()) {
    return;
  }
  m_Accepting = false;
  // Let workers hand over the blocks they already reserved, then end the stream
  while (m_InFlight.load() > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  if (!m_Done) {
    WaveBlock end;
    end.nEvents = kEndOfStream;
    m_Queue.push(std::move(end));
  }
  m_Writer.join();
  m_Queue.close();
}

Recorder::Stats Recorder::stats() const {
  Stats s;
  s.active = m_Writer.joinable() && !m_Done;
  s.events = m_Written;
  s.target = m_Target;
  s.fileBytes = m_FileBytes;
  s.floatBytes = m_FloatBytes;
  s.path = m_Path;
  std::lock_guard<std::mutex> lock(m_Mutex);
  s.seconds = s.active ? std::chrono::duration<double>(std::chrono::steady_clock::now() - m_Start).count() : m_Seconds;
  return s;
}

} // namespace sipmgui
