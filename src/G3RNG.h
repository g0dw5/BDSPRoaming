//
// Created by 王嵩 on 2022/9/19.
//

#ifndef ROAMINGID_SRC_G3RNG_H_
#define ROAMINGID_SRC_G3RNG_H_

#include <vector>

#include "definitions.h"

class LCRNG
{
 public:
  LCRNG(uint f_mult, uint f_add, uint r_mult, uint r_add)
  {
    Mult = f_mult;
    Add = f_add;
    rMult = r_mult;
    rAdd = r_add;
  }

 public:
  uint Next(uint seed) const { return (seed * Mult) + Add; }
  uint Prev(uint seed) const { return (seed * rMult) + rAdd; }

  uint Advance(uint seed, int frames) const
  {
    for (int i = 0; i < frames; i++)
      seed = Next(seed);
    return seed;
  }
  uint Reverse(uint seed, int frames) const
  {
    for (int i = 0; i < frames; i++)
      seed = Prev(seed);
    return seed;
  }

 protected:
  // Forward(const)
  uint Mult;
  uint Add;

  // Reverse(const)
  uint rMult;
  uint rAdd;
};

class RNG : public LCRNG
{
 public:
  RNG(uint f_mult, uint f_add, uint r_mult, uint r_add)
      : LCRNG(f_mult, f_add, r_mult, r_add)
  {
    // Set up bruteforce utility
    k2 = f_mult << 8;
    k0g = f_mult * f_mult;
    k2s = k0g << 8;

    // Populate Meet Middle Arrays
    uint k4g = f_add * (f_mult + 1);  // 1,3's multiplier
    for (uint i = 0; i <= maxByteSize; i++)
    {
      SetFlagData(i, f_mult, f_add, flags, low8);  // 1,2
      SetFlagData(i, k0g, k4g, g_flags, g_low8);   // 1,3
    }

    t0 = f_add - 0xFFFFU;
    t1 = 0xFFFFL * ((long)f_mult + 1);
  }

 public:
  std::vector<uint> RecoverLower16Bits(uint first, uint second)
  {
    std::vector<uint> result;
    uint k1 = second - (first * Mult);
    for (uint i = 0, k3 = k1; i <= 255; ++i, k3 -= k2)
    {
      ushort val = (ushort)(k3 >> 16);
      if (flags[val])
        result.push_back(Prev(first | i << 8 | low8[val]));
    }
    return result;
  }
  std::vector<uint> RecoverLower16BitsGap(uint first, uint third)
  {
    std::vector<uint> result;
    uint k1 = third - (first * k0g);
    for (uint i = 0, k3 = k1; i <= 255; ++i, k3 -= k2s)
    {
      ushort val = (ushort)(k3 >> 16);
      if (g_flags[val])
        result.push_back(Prev(first | i << 8 | g_low8[val]));
    }
    return result;
  }
  std::vector<uint> RecoverLower16BitsEuclid16(uint first, uint second)
  {
    const int bitshift = 32;
    const long inc = 1L << bitshift;
    return GetPossibleSeedsEuclid(first, second, bitshift, inc);
  }
  std::vector<uint> RecoverLower16BitsEuclid15(uint first, uint second)
  {
    const int bitshift = 31;
    const long inc = 1L << bitshift;
    return GetPossibleSeedsEuclid(first, second, bitshift, inc);
  }

 private:
  static void SetFlagData(uint i, uint mult, uint add, bool* f, byte* v)
  {
    // the second rand() also has 16 bits that aren't known. It is a 16 bit
    // value added to either side. to consider these bits and their impact, they
    // can at most increment/decrement the result by 1. with the current calc
    // setup, the search loop's calculated value may be -1 (loop does
    // subtraction) since LCGs are linear (hence the name), there's no values in
    // adjacent cells. (no collisions) if we mark the prior adjacent cell, we
    // eliminate the need to check flags twice on each loop.
    uint right = (mult * i) + add;
    ushort val = (ushort)(right >> 16);

    f[val] = true;
    v[val] = (byte)i;
    --val;
    f[val] = true;
    v[val] = (byte)i;
    // now the search only has to access the flags array once per loop.
  }

  std::vector<uint> GetPossibleSeedsEuclid(uint first, uint second,
                                           int bitshift, long inc)
  {
    std::vector<uint> result;
    long t = second - (Mult * first) - t0;
    long kmax = (((t1 - t) >> bitshift) << bitshift) + t;
    for (long k = t; k <= kmax; k += inc)
    {
      // compute modulo in steps for reuse in yielded value (x % Mult)
      long fix = k / Mult;
      long remainder = k - (Mult * fix);
      if (remainder >> 16 == 0)
        result.push_back(Prev(first | (uint)fix));
    }
    return result;
  }

 private:
  // Bruteforce cache for searching seeds
  static constexpr int cacheSize = 1 << 16;
  static constexpr int maxByteSize = 255;
  // 1,2 (no gap)(readonly)
  uint k2;  // Mult<<8
  byte low8[cacheSize]{};
  bool flags[cacheSize]{};
  // 1,3 (single gap)(readonly)
  uint k0g;  // Mult*Mult
  uint k2s;  // Mult*Mult<<8
  byte g_low8[cacheSize]{};
  bool g_flags[cacheSize]{};

  // Euclidean division approach(readonly)
  long t0;  // Add - 0xFFFF
  long t1;  // 0xFFFF * ((long)Mult + 1)
};

// clang-format off
/// <summary> LCRNG used for Encryption and mainline game RNG calls. </summary>
static RNG lcrng(0x41C64E6D, 0x00006073, 0xEEB9EB65, 0x0A3561A1);
/// <summary> LCRNG used by Colosseum & XD for game RNG calls. </summary>
static RNG xdrng(0x000343FD, 0x00269EC3, 0xB9B33155, 0xA170F641);
/// <summary> Alternate LCRNG used by mainline game RNG calls to disassociate the seed from the <see cref="LCRNG"/>, for anti-shiny and other purposes. </summary>
static LCRNG arng(0x6C078965, 0x00000001, 0x9638806D, 0x69C77F93);
// clang-format on

enum class PIDType
{
  Method_1,
  Method_2,
  Method_3,
  Method_4,

  Method_1_Unown,
  Method_2_Unown,
  Method_3_Unown,
  Method_4_Unown,

  Method_1_Roamer,
};

#endif  // ROAMINGID_SRC_G3RNG_H_
