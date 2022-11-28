//
// Created by 王嵩 on 2022/11/27.
//

#ifndef ROAMINGID_SRC_TERAN_FINDER_H_
#define ROAMINGID_SRC_TERAN_FINDER_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Xoroshiro128Plus.h"
#include "definitions.h"

class TeranFinder
{
 public:
  TeranFinder(std::string encounter_csv);

 public:
  void FindAllResult();

 private:
  struct Result
  {
    bool is_scarlet{};
    bool is_black{};
    uint32_t seed{};
    uint32_t star_count{};
    uint32_t teran_type;
    uint32_t species{0};
    uint32_t ec;
    uint32_t tidsid;
    uint32_t pid;
    uint32_t shiny_type;
    IVs ivs;
  };
  std::vector<Result> result_array_;

  struct Encounter
  {
    uint32_t species;
    uint32_t stars;

    int rand_rate_min_scarlet;
    int rand_rate_min_violet;

    uint32_t rand_rate;
  };
  std::unordered_map<uint32_t, std::vector<Encounter>> encounter_hash_;

 private:
  void get_species_and_teran_type(bool is_scarlet, bool is_black, uint32_t seed,
                                  uint32_t& star_count, uint32_t& teran_type,
                                  std::vector<uint32_t>& species_array);
  void generate_pkm_info(uint32_t seed, int miniv, Result& result);
};

#endif  // ROAMINGID_SRC_TERAN_FINDER_H_
