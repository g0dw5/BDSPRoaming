//
// Created by 王嵩 on 2022/5/26.
//

#ifndef ROAMINGID__RNG_POKEMON_FINDER_H_
#define ROAMINGID__RNG_POKEMON_FINDER_H_

#include <memory>

#include "definitions.h"

class IRNGPokemonFinder
{
 public:
  IRNGPokemonFinder(const ITrainerID& trainer, const IVs& ivs, uint seed,
                    Shiny shiny_type_you_want, int flawless_count)
      : trainer_(trainer),
        expect_ivs_(ivs),
        seed_(seed),
        shiny_type_you_want_(shiny_type_you_want),
        flawless_count_(flawless_count)
  {
    pkm_.SID = trainer.SID;
    pkm_.TID = trainer.TID;
  }
  virtual ~IRNGPokemonFinder() = default;

 public:
  // 验证IV/闪是否符合条件,生成完EC/PID
  virtual bool Step1IsSatisfied() = 0;
  // 补完信息
  virtual const PKM& Step2GetPokemon() = 0;

 public:
  static uint GetOID(int tid, int sid) { return (uint)((sid << 16) | tid); }

  static uint GetShinyXor(uint pid, uint oid)
  {
    uint _xor = pid ^ oid;
    return (_xor ^ (_xor >> 16)) & 0xFFFF;
  }

  static Shiny GetShinyType(uint _xor)
  {
    if (_xor == 0)
    {
      return Shiny::kSquare;
    }
    else if (_xor < 16)
    {
      return Shiny::kStar;
    }
    else
    {
      return Shiny::kNone;
    }
  }

 protected:
  uint seed_{};
  const ITrainerID& trainer_;
  const IVs& expect_ivs_;
  const int flawless_count_;
  const Shiny shiny_type_you_want_;

  PKM pkm_;

  static constexpr uint kFlawlessValue = 31;
  static constexpr int kUnsetIV = -1;
};

class RNGPokemonFinderFactory
{
 public:
  static std::unique_ptr<IRNGPokemonFinder> CreateFinder(
      RNDType type, const ITrainerID& trainer, const IVs& ivs, uint seed,
      Shiny shiny_type_you_want, int flawless_count);
};

#endif  // ROAMINGID__RNG_POKEMON_FINDER_H_
