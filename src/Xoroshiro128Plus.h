//
// Created by 王嵩 on 2022/5/26.
//

#ifndef ROAMINGID__XOROSHIRO128PLUS_H_
#define ROAMINGID__XOROSHIRO128PLUS_H_

#include "definitions.h"

struct Xoroshiro128Plus
{
 public:
  Xoroshiro128Plus(ulong s0 = XOROSHIRO_CONST0, ulong s1 = XOROSHIRO_CONST)
  {
    this->s0 = s0;
    this->s1 = s1;
  }

  ulong NextInt(ulong MOD = 0xFFFFFFFF)
  {
    ulong mask = GetBitmask(MOD);
    ulong res;
    do
    {
      res = Next() & mask;
    } while (res >= MOD);
    return res;
  }

 private:
  ulong s0, s1;

  static constexpr ulong XOROSHIRO_CONST0 = 0x0F4B17A579F18960;
  static constexpr ulong XOROSHIRO_CONST = 0x82A2B175229D6A5B;

 private:
  ulong GetBitmask(ulong x)
  {
    x--;  // comment out to always take the next biggest power of two, even if x
          // is already a power of two
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x;
  }

  ulong Next()
  {
    ulong _s0 = s0;
    ulong _s1 = s1;
    ulong result = _s0 + _s1;

    _s1 ^= _s0;
    // Final calculations and store back to fields
    s0 = RotateLeft(_s0, 24) ^ _s1 ^ (_s1 << 16);
    s1 = RotateLeft(_s1, 37);

    return result;
  }

  ulong RotateLeft(ulong x, int k) { return (x << k) | (x >> (64 - k)); }
};

#endif  // ROAMINGID__XOROSHIRO128PLUS_H_
