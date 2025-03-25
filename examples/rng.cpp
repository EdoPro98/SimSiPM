#include <cstdint>
#include <sipm/SiPMRandom.h>

int main() {

  sipm::SiPMRng::Xorshift256plus rng;

  uint64_t seed = 1234567890ULL;

  rng.seed(seed);

  for (int i = 0; i < 10; ++i) {
    const uint64_t x = rng();
    std::cout << x << std::endl;
  }

  return 0;
}
