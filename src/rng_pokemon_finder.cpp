//
// Created by 王嵩 on 2022/5/26.
//

#include "rng_pokemon_finder.h"

#include "overworld_finder.h"
#include "roaming_finder.h"

std::unique_ptr<IRNGPokemonFinder> RNGPokemonFinderFactory::CreateFinder(
    RNDType type, const ITrainerID& trainer, const IVs& ivs, uint seed,
    Shiny shiny_type_you_want, int flawless_count)
{
  switch (type)
  {
    case RNDType::kSWSHOverworld:
      return std::make_unique<OverworldFinder>(
          trainer, ivs, seed, shiny_type_you_want, flawless_count);
    case RNDType::kBDSPRoaming:
      return std::make_unique<RoamingFinder>(
          trainer, ivs, seed, shiny_type_you_want, flawless_count);
    default:
      return nullptr;
  }
}
