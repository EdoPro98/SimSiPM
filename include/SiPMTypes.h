#ifndef H_SIPM_SIPMTYPES
#define H_SIPM_SIPMTYPES

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace sipm {
/** @brief Custom implementation of @ref std::pair
 * This is a simple custom implementation of std::pair
 * but with no run-time checks to improve performance
 */
template <typename T, typename U = T>
struct pair {
  T first;
  U second;
  constexpr pair(T x, U y) : first(x), second(y) {}
  pair() = default;
};

/**
 * @class AlignedAllocator
 * @brief Allocator for aligned memory
 *
 * The AlignedAllocator class template is an allocator that
 * performs memory allocation aligned by the specified value.
 *
 * @tparam T type of objects to allocate.
 * @tparam Align alignment in bytes.
 */
template <class T, size_t Align>
class AlignedAllocator {
public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  static constexpr size_t alignment = Align;

  template <class U>
  struct rebind {
    using other = AlignedAllocator<U, Align>;
  };

  AlignedAllocator() noexcept;
  AlignedAllocator(const AlignedAllocator& rhs) noexcept;

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align>& rhs) noexcept;

  pointer address(reference) noexcept;
  const_pointer address(const_reference) const noexcept;

  pointer allocate(size_type n, const void* hint = 0);
  void deallocate(pointer p, size_type n);

  constexpr size_type max_size() const noexcept;
  constexpr size_type size_max() const noexcept;

  template <class U, class... Args>
  void construct(U* p, Args&&... args);

  template <class U>
  void destroy(U* p);
};

template <class T1, size_t Align1, class T2, size_t Align2>
constexpr bool operator==(const AlignedAllocator<T1, Align1>& lhs, const AlignedAllocator<T2, Align2>& rhs) noexcept;

template <class T1, size_t Align1, class T2, size_t Align2>
constexpr bool operator!=(const AlignedAllocator<T1, Align1>& lhs, const AlignedAllocator<T2, Align2>& rhs) noexcept;

/**
 * Default constructor.
 */
template <class T, size_t A>
inline AlignedAllocator<T, A>::AlignedAllocator() noexcept {}

/**
 * Copy constructor.
 */
template <class T, size_t A>
inline AlignedAllocator<T, A>::AlignedAllocator(const AlignedAllocator&) noexcept {}

/**
 * Extended copy constructor.
 */
template <class T, size_t A>
template <class U>
inline AlignedAllocator<T, A>::AlignedAllocator(const AlignedAllocator<U, A>&) noexcept {}

/**
 * Returns the actual address of @c r even in presence of overloaded @c operator&.
 * @param r the object to acquire address of.
 * @return the actual address of @c r.
 */
template <class T, size_t A>
inline auto AlignedAllocator<T, A>::address(reference r) noexcept -> pointer {
  return &r;
}

/**
 * Returns the actual address of @c r even in presence of overloaded \c operator&.
 * @param r the object to acquire address of.
 * @return the actual address of @c r.
 */
template <class T, size_t A>
inline auto AlignedAllocator<T, A>::address(const_reference r) const noexcept -> const_pointer {
  return &r;
}

/**
 * Allocates <tt>n * sizeof(T)</tt> bytes of uninitialized memory, aligned by @c A.
 * The alignment may require some extra memory allocation for padding.
 * @param n the number of objects to allocate storage for.
 * @param hint unused parameter provided for standard compliance.
 * @return a pointer to the first byte of a memory block suitably aligned and sufficient to
 * hold an array of @c n objects of type @c T.
 */
template <class T, size_t A>
inline auto AlignedAllocator<T, A>::allocate(size_type n, const void*) -> pointer {
  const pointer res = reinterpret_cast<pointer>(aligned_alloc(A, sizeof(T) * n));
  if (res == nullptr)
    throw std::bad_alloc();
  return res;
}

/**
 * Deallocates the storage referenced by the pointer p, which must be a pointer obtained by
 * an earlier call to allocate(). The argument \c n must be equal to the first argument of the call
 * to allocate() that originally produced \c p; otherwise, the behavior is undefined.
 * @param p pointer obtained from allocate().
 * @param n number of objects earlier passed to allocate().
 */
template <class T, size_t A>
inline void AlignedAllocator<T, A>::deallocate(pointer p, size_type) {
  free(p);
}

/**
 * Returns the maximum theoretically possible value of @c n, for which the
 * call allocate(n, 0) could succeed.
 * @return the maximum supported allocated size.
 */
template <class T, size_t A>
constexpr auto AlignedAllocator<T, A>::max_size() const noexcept -> size_type {
  return size_type(-1) / sizeof(T);
}

/**
 * Constructs an object of type @c T in allocated uninitialized memory
 * pointed to by @c p, using placement-new.
 * @param p pointer to allocated uninitialized memory.
 * @param args the constructor arguments to use.
 */
template <class T, size_t A>
template <class U, class... Args>
inline void AlignedAllocator<T, A>::construct(U* p, Args&&... args) {
  new ((void*)p) U(std::forward<Args>(args)...);
}

/**
 * Calls the destructor of the object pointed to by @c p.
 * @param p pointer to the object that is going to be destroyed.
 */
template <class T, size_t A>
template <class U>
inline void AlignedAllocator<T, A>::destroy(U* p) {
  p->~U();
}

/**
 * Compares two aligned memory allocator for equality. Since allocators
 * are stateless, return @c true iff <tt>A1 == A2</tt>.
 * @param lhs AlignedAllocator to compare.
 * @param rhs AlignedAllocator to compare.
 * @return true if the allocators have the same alignment.
 */
template <class T1, size_t A1, class T2, size_t A2>
constexpr bool operator==(const AlignedAllocator<T1, A1>& lhs, const AlignedAllocator<T2, A2>& rhs) noexcept {
  return lhs.alignment == rhs.alignment;
}

/**
 * Compares two aligned memory allocator for inequality. Since allocators
 * are stateless, return @c true iff <tt>A1 != A2</tt>.
 * @param lhs AlignedAllocator to compare.
 * @param rhs AlignedAllocator to compare.
 * @return true if the allocators have different alignments.
 */
template <class T1, size_t A1, class T2, size_t A2>
constexpr bool operator!=(const AlignedAllocator<T1, A1>& lhs, const AlignedAllocator<T2, A2>& rhs) noexcept {
  return !(lhs == rhs);
}

/** SiPMVector is an high performance version of std::vector<T>.
 * SiPMVector uses an aligned allocator that allocates memory
 * to 64 bits boundaries. This allows more efficient cache usage since
 * 64 bits is the cache line size on most x86_64 architectures.
 * Also it allows the compiler to generate AVX2 operations for aligned
 * data like `vmovapd` instead of `vmovupd`.
 */
template <typename T, int N = 64>
using SiPMVector = std::vector<T, AlignedAllocator<T, N>>;
} // namespace sipm
#endif // H_SIPM_SIPMTYPES
