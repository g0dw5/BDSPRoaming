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
  TeranFinder();

 public:
  void FindAllResult();

 private:
  struct Result
  {
    uint32_t seed{};
    uint32_t teran_type;
    uint32_t star_count{};
    // 一个seed可以在不同版本出不同的物种(这样也能只算一次ivs)
    uint32_t scarlet_species{0};
    uint32_t violet_species{0};
    uint32_t ec;
    uint32_t tidsid;
    uint32_t pid;
    uint32_t shiny_type;
    IVs ivs;
  };
  std::vector<Result> result_array_;

  std::vector<std::vector<uint32_t>>
      scarlet_species_;  // 第一层是几星(0预留空位),第二层是该等级下宝可梦数量
  std::vector<std::vector<uint32_t>>
      violet_species_;  // 第一层是几星(0预留空位),第二层是该等级下宝可梦数量

 private:
  void generate_info(uint32_t seed, std::vector<Result>& result_array);
  void generate_pkm_info(uint32_t seed, int miniv, Result& result);
};

#endif  // ROAMINGID_SRC_TERAN_FINDER_H_
