#include "SiPMRandom.h"
#include <assert.h>
#include <math.h>

using namespace sipm;
using namespace std;
int main(int argc, char const *argv[]) {
  static const int N = 1000000;

  SiPMRandom rng;

  int i = N;
  while (i > 0) {
    double x = rng.randPoisson(10);
    assert(x - trunc(x) == 0);
    assert(x >= 0);
    --i;
  }

  return 0;
}
