#include <math.h>
#include <stdint.h>
#include <random>


#ifndef SIPM_RANDOM_H
#define SIPM_RANDOM_H

namespace sipm{
static std::random_device rd;
static std::mt19937 rng(rd());
static constexpr double MT_MAX = rng.max();
static constexpr double TWO_PI = 2 * M_PI;
static constexpr double UINT64_MAXD = UINT64_MAX;
static constexpr double UINT32_MAXD = UINT32_MAX;

inline uint32_t xorshift32();
inline uint64_t xorshift64();
inline uint64_t xoroshiro256p();

inline double Rand();
inline uint32_t randInteger(const uint32_t);
double randGaussian(const double, const double);
double randExponential(const double);
uint32_t randPoisson(const double mu);

// Xorshift 32 bit
static uint32_t x32=rng();
inline uint32_t xorshift32(){
  x32 ^= x32 << 13;
  x32 ^= x32 >> 17;
  x32 ^= x32 << 5;

  return x32;
}

// Xorshift 32 bit
static uint64_t x64=rng();
inline uint64_t xorshift64(){
  x64 ^= x64 << 13;
  x64 ^= x64 >> 17;
  x64 ^= x64 << 5;

  return x64;
}

// Xoroshiro256+
static uint64_t x256[4]={rng(),rng(),rng(),rng()};
inline uint64_t xoroshiro256p(){
  const uint64_t result = x256[0] + x256[3];
  const uint64_t t = x256[1] << 17;

  x256[2] ^= x256[0];
  x256[3] ^= x256[1];
  x256[1] ^= x256[2];
  x256[0] ^= x256[3];

  x256[2] ^= t;
  x256[3] = (x256[3] << 45) | (x256[3] >> (64 - 45));

  return result;
}


//Uniform random in [0,1)
inline double Rand(){
  // return rng() / MT_MAX;
  return xorshift32() / UINT32_MAXD;
  // return xoroshiro256p() / UINT64_MAXD;
}


//Integer random in range [0,max]
inline uint32_t randInteger(const uint32_t max){
  return xorshift32() % max;
}

} /* NAMESPACE_SIPM */
#endif /* SIPM_RANDOM_H */
