#ifndef H_SIPM_SIPMTYPES
#define H_SIPM_SIPMTYPES

#include <cstdint>
#include <cstdlib>
#include <cstddef> // for std::size_t
#include <cstring> // for std::memcpy, std::memmove
#include <utility> // for std::move, std::forward
#include <iostream>

namespace sipm {
/** @brief Custom implementation of @ref std::pair
 * This is a simple custom implementation of std::pair
 * but with no run-time checks to improve performance
 */
template <typename T, typename U = T>
struct pair {
  T first{};
  U second{};

  // Default constructor
  constexpr pair() noexcept = default;

  // Parameterized constructor
  constexpr pair(const T& x, const U& y) noexcept : first(x), second(y) {}

  // Forwarding constructor for perfect forwarding
  template <typename X, typename Y>
  constexpr pair(X&& x, Y&& y) noexcept : first(std::forward<X>(x)), second(std::forward<Y>(y)) {}

  // Copy constructor
  constexpr pair(const pair& other) noexcept = default;

  // Move constructor
  constexpr pair(pair&& other) noexcept = default;

  // Copy assignment
  constexpr pair& operator=(const pair& other) noexcept = default;

  // Move assignment
  constexpr pair& operator=(pair&& other) noexcept = default;

  // Swap function
  void swap(pair& other) noexcept {
    std::swap(first, other.first);
    std::swap(second, other.second);
  }
};

template <size_t N = 64>
inline void* sipmAlloc(const size_t bytes) {
  static_assert((N & (N - 1)) == 0, "Alignment must be a power of 2");
  const size_t alignedBytes = (bytes + N - 1) & ~(N - 1);
#ifdef __AVX512F__
  return aligned_alloc(N, alignedBytes);
#else
  return malloc(alignedBytes);
#endif
}

inline void sipmFree(void* ptr) { free(ptr); }

template <typename T, std::size_t N = 3>
class SiPMSmallVector {
private:
  alignas(64) T m_LocalStorage[N];
    T* m_HeapStorage = nullptr;
    size_t m_Size = 0;
    size_t m_Capacity = N;

    inline bool isLocal() const noexcept {
        return m_Capacity == N;
    }

    void moveToHeap() {
      m_HeapStorage = (T*)malloc(2*N*sizeof(T));
        memcpy(m_HeapStorage, m_LocalStorage, m_Size * sizeof(T));
        m_Capacity = 2 * N;
    }

    void growHeap() {
      m_Capacity *= 2;
      m_HeapStorage = (T*)realloc(m_HeapStorage, m_Capacity  * sizeof(T));
    }

    T* data() noexcept { return isLocal() ? m_LocalStorage : m_HeapStorage; }

    const T* data() const noexcept { return isLocal() ? m_LocalStorage : m_HeapStorage; }

  public:
    SiPMSmallVector() = default;

    ~SiPMSmallVector() {
      if (m_HeapStorage != nullptr) {
        free(m_HeapStorage);
      }
    }

    void push_back(const T& value) {
      if (m_Size == m_Capacity ) {
        if (isLocal()) {
          moveToHeap();
        } else {
          growHeap();
        }
      }
      data()[m_Size++] = value;
    }

    void push_back(T&& value) {
        if (m_Size == m_Capacity ) {
            if (isLocal()) {
                moveToHeap();
            } else {
                growHeap();
            }
        }
        data()[m_Size++] = std::move(value);
    }

    void pop_back() {
        --m_Size;
    }

    T& operator[](std::size_t index) noexcept {
        return data()[index];
    }

    const T& operator[](std::size_t index) const noexcept {
        return data()[index];
    }

    size_t size() const noexcept {
        return m_Size;
    }

    void clear() {
        m_Size = 0;
        if (!isLocal()) {
            free(m_HeapStorage);
            m_HeapStorage = nullptr;
            m_Capacity  = N;
        }
    }

    T* begin() noexcept { return data(); }
    T* end() noexcept { return data() + m_Size; }

    const T* begin() const noexcept { return data(); }
    const T* end() const noexcept { return data() + m_Size; }

    const T* cbegin() const noexcept { return data(); }
    const T* cend() const noexcept { return data() + m_Size; }
};

} // namespace sipm
#endif // H_SIPM_SIPMTYPES
