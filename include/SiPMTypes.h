#ifndef H_SIPM_SIPMTYPES
#define H_SIPM_SIPMTYPES

#include <cstdint>
#include <utility> // for std::move, std::forward

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

} // namespace sipm
#endif // H_SIPM_SIPMTYPES
