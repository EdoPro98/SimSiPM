#ifndef H_SIPM_SIPMTYPES
#define H_SIPM_SIPMTYPES

#include <cstdint>
#include <cstdlib>
#include <cstddef> // for std::size_t
#include <cstring> // for std::memcpy, std::memmove
#include <utility> // for std::move, std::forward
#include <iostream>

namespace sipm {
/** @brief Custom implementation of std::pair
 * This is a simple custom implementation of std::pair
 * but with no run-time checks to improve performance
 * @tparam T Type of the first element
 * @tparam U Type of the second element, same as T by default
 */
template <typename T, typename U = T>
struct pair {
  T first{};  ///< First element
  U second{};  ///< Second element

  // Default constructor
  /// @brief Value-initializes both elements
  constexpr pair() noexcept = default;

  // Parameterized constructor
  /// @brief Copies the elements
  /// @param x First element
  /// @param y Second element
  constexpr pair(const T& x, const U& y) noexcept : first(x), second(y) {}

  // Forwarding constructor for perfect forwarding
  template <typename X, typename Y>
  /// @brief Perfectly forwards the elements
  /// @param x First element
  /// @param y Second element
  constexpr pair(X&& x, Y&& y) noexcept : first(std::forward<X>(x)), second(std::forward<Y>(y)) {}

  // Copy constructor
  /// @brief Copy constructor
  /// @param other Pair to copy
  constexpr pair(const pair& other) noexcept = default;

  // Move constructor
  /// @brief Move constructor
  /// @param other Pair to move from
  constexpr pair(pair&& other) noexcept = default;

  // Copy assignment
  /// @brief Copy assignment
  /// @param other Pair to copy
  /// @return This pair
  constexpr pair& operator=(const pair& other) noexcept = default;

  // Move assignment
  /// @brief Move assignment
  /// @param other Pair to move from
  /// @return This pair
  constexpr pair& operator=(pair&& other) noexcept = default;

  // Swap function
  /// @brief Swaps the elements with another pair
  /// @param other Pair to swap with
  void swap(pair& other) noexcept {
    std::swap(first, other.first);
    std::swap(second, other.second);
  }
};

/// @brief Allocates memory for a buffer
/// With AVX512 the memory is aligned to N bytes and the size rounded up to a multiple of N; otherwise plain malloc is used.
/// Release it with sipmFree().
/// @tparam N Alignment in bytes, a power of 2
/// @param bytes Requested size in bytes
/// @return Pointer to the allocated memory
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

/// @brief Releases memory obtained with sipmAlloc()
/// @param ptr Pointer to the memory
inline void sipmFree(void* ptr) { free(ptr); }

/// @brief Vector that keeps its first N elements inside the object and moves to the heap when it grows
/// Meant for trivially copyable types (memcpy/realloc are used). Elements are not constructed or destroyed.
/// @tparam T Type of the elements
/// @tparam N Number of elements stored inside the object
template <typename T, std::size_t N = 3>
class SiPMSmallVector {
private:
  alignas(64) T m_LocalStorage[N];  ///< Inline storage for the first N elements
    T* m_HeapStorage = nullptr;  ///< Heap storage, used when more than N elements are stored
    size_t m_Size = 0;  ///< Number of stored elements
    size_t m_Capacity = N;  ///< Current capacity, equal to N while the inline storage is used

    /// @brief Whether the elements are in the inline storage
    /// @return true if no heap storage is in use
    inline bool isLocal() const noexcept {
        return m_Capacity == N;
    }

    /// @brief Moves the elements from the inline storage to a heap buffer of twice the capacity
    void moveToHeap() {
      m_HeapStorage = (T*)malloc(2*N*sizeof(T));
        memcpy(m_HeapStorage, m_LocalStorage, m_Size * sizeof(T));
        m_Capacity = 2 * N;
    }

    /// @brief Doubles the capacity of the heap buffer
    void growHeap() {
      m_Capacity *= 2;
      m_HeapStorage = (T*)realloc(m_HeapStorage, m_Capacity  * sizeof(T));
    }

    /// @brief Pointer to the first element
    /// @return Pointer to the elements, wherever they are stored
    T* data() noexcept { return isLocal() ? m_LocalStorage : m_HeapStorage; }

    /// @brief Pointer to the first element
    /// @return Const pointer to the elements, wherever they are stored
    const T* data() const noexcept { return isLocal() ? m_LocalStorage : m_HeapStorage; }

  public:
    /// @brief Creates an empty vector
    SiPMSmallVector() = default;

    ~SiPMSmallVector() {
      if (m_HeapStorage != nullptr) {
        free(m_HeapStorage);
      }
    }

    /// @brief Copy constructor
    /// @param other Vector to copy
    SiPMSmallVector(const SiPMSmallVector& other) noexcept
      : m_Size(other.m_Size), m_Capacity(other.m_Capacity) {
      if (other.isLocal()) {
        m_HeapStorage = nullptr;
        memcpy(m_LocalStorage, other.m_LocalStorage, m_Size * sizeof(T));
      } else {
        m_HeapStorage = static_cast<T*>(malloc(m_Capacity * sizeof(T)));
        memcpy(m_HeapStorage, other.m_HeapStorage, m_Size * sizeof(T));
      }
    }

    /// @brief Copy assignment
    /// @param other Vector to copy
    /// @return This vector
    SiPMSmallVector& operator=(const SiPMSmallVector& other) noexcept {
      if (this == &other) { return *this; }
      if (m_HeapStorage != nullptr) {
        free(m_HeapStorage);
        m_HeapStorage = nullptr;
      }
      m_Size = other.m_Size;
      m_Capacity = other.m_Capacity;
      if (other.isLocal()) {
        memcpy(m_LocalStorage, other.m_LocalStorage, m_Size * sizeof(T));
      } else {
        m_HeapStorage = static_cast<T*>(malloc(m_Capacity * sizeof(T)));
        memcpy(m_HeapStorage, other.m_HeapStorage, m_Size * sizeof(T));
      }
      return *this;
    }

    /// @brief Move constructor, leaves @p other empty
    /// @param other Vector to move from
    SiPMSmallVector(SiPMSmallVector&& other) noexcept
      : m_Size(other.m_Size), m_Capacity(other.m_Capacity) {
      if (other.isLocal()) {
        m_HeapStorage = nullptr;
        memcpy(m_LocalStorage, other.m_LocalStorage, m_Size * sizeof(T));
      } else {
        m_HeapStorage = other.m_HeapStorage;
        other.m_HeapStorage = nullptr;
        other.m_Capacity = N;
      }
      other.m_Size = 0;
    }

    /// @brief Move assignment, leaves @p other empty
    /// @param other Vector to move from
    /// @return This vector
    SiPMSmallVector& operator=(SiPMSmallVector&& other) noexcept {
      if (this == &other) { return *this; }
      if (m_HeapStorage != nullptr) {
        free(m_HeapStorage);
        m_HeapStorage = nullptr;
      }
      m_Size = other.m_Size;
      m_Capacity = other.m_Capacity;
      if (other.isLocal()) {
        memcpy(m_LocalStorage, other.m_LocalStorage, m_Size * sizeof(T));
      } else {
        m_HeapStorage = other.m_HeapStorage;
        other.m_HeapStorage = nullptr;
        other.m_Capacity = N;
      }
      other.m_Size = 0;
      return *this;
    }

    /// @brief Appends a copy of an element, growing the storage if needed
    /// @param value Element to append
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

    /// @brief Appends an element, growing the storage if needed
    /// @param value Element to append
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

    /// @brief Removes the last element
    void pop_back() {
        --m_Size;
    }

    /// @brief Access to an element, no bounds check
    /// @param index Position of the element
    /// @return Reference to the element
    T& operator[](std::size_t index) noexcept {
        return data()[index];
    }

    /// @brief Access to an element, no bounds check
    /// @param index Position of the element
    /// @return Const reference to the element
    const T& operator[](std::size_t index) const noexcept {
        return data()[index];
    }

    /// @brief Number of elements
    /// @return Number of elements stored
    size_t size() const noexcept {
        return m_Size;
    }

    /// @brief Removes all the elements and releases the heap storage
    void clear() {
        m_Size = 0;
        if (!isLocal()) {
            free(m_HeapStorage);
            m_HeapStorage = nullptr;
            m_Capacity  = N;
        }
    }

    /// @brief Iterator to the first element
    /// @return Pointer to the first element
    T* begin() noexcept { return data(); }
    /// @brief Iterator past the last element
    /// @return Pointer past the last element
    T* end() noexcept { return data() + m_Size; }

    /// @brief Iterator to the first element
    /// @return Const pointer to the first element
    const T* begin() const noexcept { return data(); }
    /// @brief Iterator past the last element
    /// @return Const pointer past the last element
    const T* end() const noexcept { return data() + m_Size; }

    /// @brief Const iterator to the first element
    /// @return Const pointer to the first element
    const T* cbegin() const noexcept { return data(); }
    /// @brief Const iterator past the last element
    /// @return Const pointer past the last element
    const T* cend() const noexcept { return data() + m_Size; }
};

} // namespace sipm
#endif // H_SIPM_SIPMTYPES
