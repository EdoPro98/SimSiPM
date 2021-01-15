#include "SiPMRandom.h"
#include <assert.h>
#include <stdint.h>

using namespace sipm;
using namespace std;
int main(int argc, char const* argv[]) {
  int k, s;

  SiPMRng::Xorshift256plus rng;

  k = 1000;
  s = 1000;
  while (k > 0) {
    uint64_t temp[s];
    for (int j = 0; j < s; ++j) { temp[j] = rand(); }

    for (int i = 0; i < s; ++i) {
      for (int j = 0; j < s; ++j) {
        if (i != j) { assert(temp[i] != temp[j]); }
      }
    }
    --k;
  }

  k = 100;
  s = 10000;
  while (k > 0) {
    uint64_t temp[s];
    for (int j = 0; j < s; ++j) { temp[j] = rng(); }

    for (int i = 0; i < s; ++i) {
      for (int j = 0; j < s; ++j) {
        if (i != j) { assert(temp[i] != temp[j]); }
      }
    }
    --k;
  }

  return 0;
}
