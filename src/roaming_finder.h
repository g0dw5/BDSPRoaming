//
// Created by 王嵩 on 2022/5/18.
//

#ifndef ROAMINGID__ROAMING_FINDER_H_
#define ROAMINGID__ROAMING_FINDER_H_

#include "Xoroshiro128Plus8b.h"
#include "rng_pokemon_finder.h"

class RoamingFinder : public IRNGPokemonFinder
{
 public:
  RoamingFinder(const ITrainerID& trainer, const IVs& ivs, uint seed,
                Shiny shiny_type_you_want, int flawless_count)
      : IRNGPokemonFinder(trainer, ivs, seed, shiny_type_you_want,
                          flawless_count),
        rnd_(seed_)
  {
    pkm_.SID = trainer.SID;
    pkm_.TID = trainer.TID;
  }
  RoamingFinder(const RoamingFinder&) = delete;
  RoamingFinder(RoamingFinder&&) = delete;
  RoamingFinder& operator=(const RoamingFinder&) = delete;
  RoamingFinder& operator=(RoamingFinder&&) = delete;
  virtual ~RoamingFinder() = default;

 public:
  // 生成假OID,PID,IVs
  bool Step1IsSatisfied() override;
  // 生成特性序号,身高,体重
  const PKM& Step2GetPokemon() override;

 private:
  Xoroshiro128Plus8b rnd_;

 private:
  static uint GetRevisedPID(uint oid_from_rand, uint pid, ITrainerID tr);
};

#endif  // ROAMINGID__ROAMING_FINDER_H_
