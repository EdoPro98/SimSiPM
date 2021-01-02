#include "SiPMRandom.h"
#include <assert.h>

using namespace sipm;
using namespace std;
int main(int argc, char const *argv[]) {
  static const int N = 1000000;

  int i = N;
  while (i > 0){
    double x = randExponential(1);
    assert(x > 0);
    --i;
  }

  i = N;
  while(i > 0){
    double x = randExponential(1);
    double y = randExponential(1);
    assert(x != y);
    i--;
  }
  return 0;
}
