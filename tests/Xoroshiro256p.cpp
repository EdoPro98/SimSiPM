#include "SiPMRandom.h"
#include <assert.h>
#include <stdint.h>

using namespace sipm;
using namespace std;
int main(int argc, char const *argv[]) {
  int k;


  k = 1000;
  while(k > 0){
    uint64_t temp[100];
    for (int j=0;j<100;++j){
      temp[j] = xoroshiro256p();
    }

    for(int i=0;i<100;++i){
      for(int j=0;j<100;++j){
        if(i != j){
          assert(temp[i] != temp[j]);
        }
      }
    }
  --k;
  }

  k = 1000;
  while(k > 0){
    uint64_t temp[1000];
    for (int j=0;j<1000;++j){
      temp[j] = xoroshiro256p();
    }

    for(int i=0;i<1000;++i){
      for(int j=0;j<1000;++j){
        if(i != j){
          assert(temp[i] != temp[j]);
        }
      }
    }
  --k;
  }

  return 0;
}
