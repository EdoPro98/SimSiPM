#ifndef SIPM_GUI_BOUNDEDQUEUE_H
#define SIPM_GUI_BOUNDEDQUEUE_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>

namespace sipmgui {

/// Fixed-capacity multi-producer / multi-consumer FIFO.
/// Producers can either block until there is room (push) or give up
/// immediately (tryPush). Closing the queue wakes every waiting thread.
template <typename T>
class BoundedQueue {
public:
  explicit BoundedQueue(size_t capacity) : m_Capacity(capacity) {}

  /// Blocks while full. Returns false if the queue has been closed.
  bool push(T&& item) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_NotFull.wait(lock, [this] { return m_Closed || m_Items.size() < m_Capacity; });
    if (m_Closed) {
      return false;
    }
    m_Items.push_back(std::move(item));
    lock.unlock();
    m_NotEmpty.notify_one();
    return true;
  }

  /// Returns false (and leaves item untouched) if the queue is full or closed.
  bool tryPush(T&& item) {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      if (m_Closed || m_Items.size() >= m_Capacity) {
        return false;
      }
      m_Items.push_back(std::move(item));
    }
    m_NotEmpty.notify_one();
    return true;
  }

  /// Blocks while empty. Returns false once the queue is closed.
  bool pop(T& out) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_NotEmpty.wait(lock, [this] { return m_Closed || !m_Items.empty(); });
    if (m_Closed) {
      return false;
    }
    out = std::move(m_Items.front());
    m_Items.pop_front();
    lock.unlock();
    m_NotFull.notify_one();
    return true;
  }

  bool tryPop(T& out) {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      if (m_Items.empty()) {
        return false;
      }
      out = std::move(m_Items.front());
      m_Items.pop_front();
    }
    m_NotFull.notify_one();
    return true;
  }

  void clear() {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_Items.clear();
    }
    m_NotFull.notify_all();
  }

  void close() {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_Closed = true;
    }
    m_NotFull.notify_all();
    m_NotEmpty.notify_all();
  }

  void reopen() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Items.clear();
    m_Closed = false;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Items.size();
  }

  bool full() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Items.size() >= m_Capacity;
  }

  size_t capacity() const { return m_Capacity; }

private:
  const size_t m_Capacity;
  std::deque<T> m_Items;
  mutable std::mutex m_Mutex;
  std::condition_variable m_NotFull;
  std::condition_variable m_NotEmpty;
  bool m_Closed = false;
};

} // namespace sipmgui
#endif
