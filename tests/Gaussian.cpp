#include "SiPMRandom.h"
#include <assert.h>
#include <math.h>

#include <iostream>

using namespace sipm;
using namespace std;
int main(int argc, char const* argv[]) {
  static const int N = (int)1e7;

  SiPMRandom rng;

  int i = N;
  uint64_t one_sigma = 0;
  uint64_t two_sigma = 0;
  uint64_t three_sigma = 0;

  while (i > 0) {
    double x = rng.randGaussian(0, 1);
    if (abs(x) < 1) { one_sigma++; }
    if (abs(x) < 2) { two_sigma++; }
    if (abs(x) < 3) { three_sigma++; }
    --i;
  }

  double p_one_sigma = 100 * (double)one_sigma / N;
  double p_two_sigma = 100 * (double)two_sigma / N;
  double p_three_sigma = 100 * (double)three_sigma / N;

  assert((p_one_sigma < 69) && (p_one_sigma > 65));
  assert((p_two_sigma < 96.45) && (p_two_sigma > 94.45));
  assert((p_three_sigma < 99.9) && (p_three_sigma > 99.5));

  i = (int)1e6;
  while (i > 0) {
    double x = rng.randGaussian(0, 1);
    double y = rng.randGaussian(0, 1);
    assert(x != y);
    --i;
  }

  return 0;
}
