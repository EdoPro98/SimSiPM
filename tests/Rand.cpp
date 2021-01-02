#include "SiPMRandom.h"
#include <assert.h>

using namespace sipm;
using namespace std;
int main(int argc, char const *argv[]) {
  static const int N = 1000000;

  int i = N;
  while (i > 0){
    double x = Rand();
    assert(x > 0);
    assert(x < 1);
    --i;
  }

  i = N;
  while(i > 0){
    double x = Rand();
    double y = Rand();
    assert(x != y);
    i--;
  }
  return 0;
}
