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
#include "PersonalTable9SV.h"

class TeranFinder
{
 public:
  TeranFinder();

 public:
  void FindAllResult();

 private:
  struct Result
  {
    uint32_t seed{};
    uint32_t star_count{};
    bool is_scarlet{};
    // EncounterTera9编号
    uint16_t encounter_index{};
    uint32_t teran_type;
    uint32_t ec;
    uint32_t tidsid;
    uint32_t pid;
    uint32_t shiny_type;
    IVs ivs;
    uint16_t ability;
    uint8_t gender;
    uint8_t nature;
    uint8_t height;
    uint8_t weight;
    uint8_t scale;
  };
  // 输出内容:朱/紫,seed,encounter_index(外键),ec,pid,shiny,iv_type(只四种),ability,gender,nature,height,weight,scale
  std::vector<Result> result_array_;

  // 第一层是几星(0预留空位),第二层是该等级下宝可梦数量,数值为EncounterTera9Table的下标
  std::vector<std::vector<uint32_t>> scarlet_species_;
  std::vector<std::vector<uint32_t>> violet_species_;

 private:
  void generate_info(uint32_t seed, std::vector<Result>& result_array);
  void generate_pkm_info(uint32_t seed, const EncounterTera9 &encounter, Result &result);
};

#endif  // ROAMINGID_SRC_TERAN_FINDER_H_
