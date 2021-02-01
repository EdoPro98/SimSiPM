#include <chrono>
#include <iostream>

using namespace std;

namespace sipm {
struct Timer {
  Timer() noexcept;
  ~Timer();
  chrono::time_point<chrono::high_resolution_clock> start;
  inline double Time() noexcept;
};

Timer::Timer() noexcept { start = std::chrono::high_resolution_clock::now(); }

Timer::~Timer() {
  auto end = std::chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  std::cout << "Timer took: " << duration.count() << " s" << '\n';
  std::cout << "Timer took: " << duration.count() * 1000 << " ms" << '\n';
  std::cout << "Timer took: " << duration.count() * 1e6 << " us" << '\n';
  std::cout << endl;
}

inline double Timer::Time() noexcept {
  auto end = std::chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  return duration.count() * 1e6;
}
}  // namespace sipm
