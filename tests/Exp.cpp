#include "SiPMRandom.h"
#include <assert.h>

using namespace sipm;
using namespace std;
int main(int argc, char const* argv[]) {
  static const int N = 1000000;

  sipm::SiPMRandom rng;
  int i = N;

  while (i > 0) {
    double x = rng.randExponential(1);
    assert(x > 0);
    --i;
  }

  i = N;
  while (i > 0) {
    double x = rng.randExponential(1);
    double y = rng.randExponential(1);
    assert(x != y);
    i--;
  }
  return 0;
}
