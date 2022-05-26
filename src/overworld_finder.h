//
// Created by 王嵩 on 2022/5/26.
//

#ifndef ROAMINGID__OVERWORLD_FINDER_H_
#define ROAMINGID__OVERWORLD_FINDER_H_

#include "Xoroshiro128Plus.h"
#include "rng_pokemon_finder.h"

class OverworldFinder : public IRNGPokemonFinder
{
 public:
  OverworldFinder(const ITrainerID& trainer, const IVs& ivs, uint seed,
                  Shiny shiny_type_you_want, int flawless_count)
      : IRNGPokemonFinder(trainer, ivs, seed, shiny_type_you_want,
                          flawless_count),
        rnd_(seed_)
  {
    pkm_.SID = trainer.SID;
    pkm_.TID = trainer.TID;
  }
  OverworldFinder(const OverworldFinder&) = delete;
  OverworldFinder(OverworldFinder&&) = delete;
  OverworldFinder& operator=(const OverworldFinder&) = delete;
  OverworldFinder& operator=(OverworldFinder&&) = delete;
  virtual ~OverworldFinder() = default;

 public:
  // 生成假OID,PID,IVs
  bool Step1IsSatisfied() override;
  // 生成特性序号,身高,体重
  const PKM& Step2GetPokemon() override;

 private:
  Xoroshiro128Plus rnd_;

 private:
  uint GetRevisedPID(uint pid, ITrainerID tr);
};

#endif  // ROAMINGID__OVERWORLD_FINDER_H_
