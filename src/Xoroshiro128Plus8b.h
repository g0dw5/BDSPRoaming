//
// Created by 王嵩 on 2022/5/18.
//

#ifndef ROAMINGID__XOROSHIRO128PLUS8B_H_
#define ROAMINGID__XOROSHIRO128PLUS8B_H_

#include "definitions.h"

struct Xoroshiro128Plus8b {
 public:
  Xoroshiro128Plus8b(ulong seed) {
    s0 = SplitMix64(seed + 0x9E3779B97F4A7C15);
    s1 = SplitMix64(seed + 0x3C6EF372FE94F82A);
  }

  uint NextUInt() {
    return (uint) (Next() >> 32);
  }

  uint NextUInt(uint max) {
    uint rnd = NextUInt();
    return rnd - ((rnd / max) * max);
  }

 private:
  ulong SplitMix64(ulong seed) {
    seed = 0xBF58476D1CE4E5B9 * (seed ^ (seed >> 30));
    seed = 0x94D049BB133111EB * (seed ^ (seed >> 27));
    return seed ^ (seed >> 31);
  }

  ulong Next() {
    ulong _s0 = s0;
    ulong _s1 = s1;
    ulong result = _s0 + _s1;
    _s1 ^= s0;

    // Final calculations and store back to fields
    s0 = RotateLeft(_s0, 24) ^ _s1 ^ (_s1 << 16);
    s1 = RotateLeft(_s1, 37);

    return result;
  }

  ulong RotateLeft(ulong x, int k) {
    return (x << k) | (x >> (64 - k));
  }

 private:
  ulong s0, s1;
};

#endif//ROAMINGID__XOROSHIRO128PLUS8B_H_
