//
// Created by 王嵩 on 2022/11/27.
//

#include "teran_finder.h"

#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#include "thread_pool.h"

TeranFinder::TeranFinder(std::string encounter_csv)
{
  std::ifstream ifstr(encounter_csv);

  std::string line;

  getline(ifstr, line);
  while (getline(ifstr, line))
  {
    std::istringstream iss(line);
    std::string elem;

    Encounter encounter;

    uint32_t elem_index{};
    while (getline(iss, elem, ','))
    {
      ++elem_index;

      switch (elem_index)
      {
        case 1:
          encounter.species = std::stoi(elem);
          break;
        case 2:
          encounter.stars = std::stoi(elem);
          break;
        case 3:
          encounter.rand_rate_min_scarlet = std::stoi(elem);
          break;
        case 4:
          encounter.rand_rate_min_violet = std::stoi(elem);
          break;
        case 5:
          encounter.rand_rate = std::stoi(elem);
          break;
        default:
          __builtin_trap();
      }
    }

    if (elem_index == 5)
    {
      encounter_hash_[encounter.stars].emplace_back(encounter);
    }
  }
}

std::string GetTeranString(uint32_t teran_type)
{
  switch (teran_type)
  {
    case 0:
      return "一般";
    case 1:
      return "格斗";
    case 2:
      return "飞行";
    case 3:
      return "毒";
    case 4:
      return "地面";
    case 5:
      return "岩石";
    case 6:
      return "虫";
    case 7:
      return "幽灵";
    case 8:
      return "钢";
    case 9:
      return "火";
    case 10:
      return "水";
    case 11:
      return "草";
    case 12:
      return "电";
    case 13:
      return "超能力";
    case 14:
      return "冰";
    case 15:
      return "龙";
    case 16:
      return "恶";
    case 17:
      return "妖精";
    default:
      __builtin_trap();
  }
}

void TeranFinder::FindAllResult()
{
  // 循环朱/紫
  //   循环是否black
  //     循环所有seed,直接决定了种族与太晶类型
  //       决定图鉴编号与太晶类型
  //       决定ec/pid/shiny/ivs

  ThreadPool thread_pool(std::thread::hardware_concurrency());
  std::list<std::future<std::vector<Result>>> futures;

  uint32_t seed_count_in_one_batch{8192};
  uint64_t batch_count = 0x100000000L / seed_count_in_one_batch;

  for (int i = 0; i < 2; ++i)
  {
    bool is_scarlet = i;
    for (int j = 0; j < 2; ++j)
    {
      bool is_black = j;

      for (uint64_t batch = 0; batch < batch_count; ++batch)
      {
        uint32_t seed_beg = seed_count_in_one_batch * batch;
        uint32_t seed_end = seed_count_in_one_batch * (batch + 1);

        futures.emplace_back(thread_pool.enqueue(
            [this, is_scarlet, is_black, seed_beg, seed_end]
            {
              std::vector<Result> result_array;

              for (ulong seed = seed_beg; seed < seed_end; ++seed)
              {
                uint32_t star_count{};
                uint32_t teran_type{};
                std::vector<uint32_t> species_array;
                int species_roll{};
                get_species_and_teran_type(is_scarlet, is_black, seed,
                                           star_count, teran_type,
                                           species_array, species_roll);
                if (species_array.size() != 1)
                  continue;

                Result result;
                result.is_scarlet = is_scarlet;
                result.is_black = is_black;
                result.seed = seed;
                result.star_count = star_count;
                result.teran_type = teran_type;
                result.species = species_array[0];

                generate_pkm_info(seed, star_count - 1, result);

                if (result.shiny_type != 1)
                  continue;

                bool is_6v =
                    (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 31 &&
                     result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                     result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 31);
                bool is_5v0a =
                    (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 0 &&
                     result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                     result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 31);
                bool is_5v0e =
                    (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 31 &&
                     result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                     result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 0);
                bool is_4v0a0e =
                    (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 0 &&
                     result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                     result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 0);
                if (!is_6v && !is_5v0a && !is_5v0e && !is_4v0a0e)
                  continue;

                result_array.emplace_back(result);
              }
              return result_array;
            }));
      }
    }
  }

  for (auto& f : futures)
  {
    const auto& result_array = f.get();
    for (const auto& result : result_array)
      result_array_.push_back(result);
  }

  std::ofstream ofstr("result_only_shiny.txt");
  ofstr << "版本,seed,星级,太晶,图鉴编号,ec,pid,是否闪光,";
  ofstr << "hp,atk,def,spa,spd,spe" << std::endl;
  for (const auto& result : result_array_)
  {
    ofstr << (result.is_scarlet ? "朱" : "紫") << ",";
    ofstr << std::hex << "0x" << result.seed << ",";
    ofstr << std::dec << result.star_count << ","
          << GetTeranString(result.teran_type) << "," << result.species << ",";
    ofstr << std::hex << "0x" << result.ec << ",0x" << result.pid << ",";
    ofstr << std::dec << result.shiny_type << ",";
    ofstr << std::dec << result.ivs.IV_HP << "," << result.ivs.IV_ATK << ","
          << result.ivs.IV_DEF << "," << result.ivs.IV_SPA << ","
          << result.ivs.IV_SPD << "," << result.ivs.IV_SPE << std::endl;
  }
}

uint GetRateTotalBaseScarlet(int star)
{
  switch (star)
  {
    case 1:
      return 5800;
    case 2:
      return 5300;
    case 3:
      return 7400;
    case 4:
      return 8800;
    case 5:
      return 9100;
    case 6:
      return 6500;
    default:
      return 0;
  }
}

uint GetRateTotalBaseViolet(int star)
{
  switch (star)
  {
    case 1:
      return 5800;
    case 2:
      return 5300;
    case 3:
      return 7400;
    case 4:
      return 8700;
    case 5:
      return 9100;
    case 6:
      return 6500;
    default:
      return 0;
  }
}

int GetStarCount(uint Difficulty, int Progress, bool IsBlack)
{
  if (IsBlack)
    return 6;

  switch (Progress)
  {
    case 0:
    {
      if (Difficulty > 80)
        return 2;
      else
        return 1;
    }
    case 1:
    {
      if (Difficulty > 70)
        return 3;
      else if (Difficulty > 30)
        return 2;
      else
        return 1;
    }
    case 2:
    {
      if (Difficulty > 70)
        return 4;
      else if (Difficulty > 40)
        return 3;
      else if (Difficulty > 20)
        return 2;
      else
        return 1;
    }
    case 3:
    {
      if (Difficulty > 75)
        return 5;
      else if (Difficulty > 40)
        return 4;
      else
        return 3;
    }
    case 4:
    {
      if (Difficulty > 70)
        return 5;
      else if (Difficulty > 30)
        return 4;
      else
        return 3;
    }
    default:
      return 1;
  }
}

void TeranFinder::get_species_and_teran_type(
    bool is_scarlet, bool is_black, uint32_t seed, uint32_t& star_count,
    uint32_t& teran_type, std::vector<uint32_t>& species_array,
    int& species_roll)
{
  Xoroshiro128Plus rng(seed);

  star_count = is_black ? 6 : GetStarCount(rng.NextInt(100), 4, is_black);

  uint total = is_scarlet ? GetRateTotalBaseScarlet(star_count)
                          : GetRateTotalBaseViolet(star_count);

  species_roll = rng.NextInt(total);

  auto iter_star = encounter_hash_.find(star_count);
  for (const auto& encounter : iter_star->second)
  {
    int minimum = is_scarlet ? encounter.rand_rate_min_scarlet
                             : encounter.rand_rate_min_violet;
    if (minimum == -1)
      continue;

    // TODO(wang.song) 改成查表优化性能
    if (abs(species_roll - minimum) < encounter.rand_rate)
    {
      species_array.push_back(encounter.species);
      // 有序只返回一个
      break;
    }
  }

  teran_type = Xoroshiro128Plus(seed).NextInt(18);
}

void TeranFinder::generate_pkm_info(uint32_t seed, int miniv, Result& result)
{
  Xoroshiro128Plus rng(seed);

  result.ec = (uint)rng.NextInt();
  result.tidsid = (uint)rng.NextInt();
  result.pid = (uint)rng.NextInt();
  result.shiny_type =
      (((result.pid >> 16) ^ (result.pid & 0xFFFF)) >> 4) ==
              (((result.tidsid >> 16) ^ (result.tidsid & 0xFFFF)) >> 4)
          ? 1
          : 0;

  static constexpr uint kFlawlessValue = 31;
  static constexpr int kUnsetIV = -1;
  std::vector<int> ivs{kUnsetIV, kUnsetIV, kUnsetIV,
                       kUnsetIV, kUnsetIV, kUnsetIV};

  // 用完美个数取index,剩下的随机个体值
  int determined = 0;
  while (determined < miniv)
  {
    int idx = (int)rng.NextInt(6);
    if (ivs[idx] != kUnsetIV)
      continue;

    ivs[idx] = kFlawlessValue;
    ++determined;
  }

  for (int idx = 0; idx < ivs.size(); ++idx)
  {
    if (ivs[idx] == kUnsetIV)
    {
      ivs[idx] = (int)rng.NextInt(kFlawlessValue + 1);
    }
  }

  result.ivs.IV_HP = ivs[0];
  result.ivs.IV_ATK = ivs[1];
  result.ivs.IV_DEF = ivs[2];
  result.ivs.IV_SPA = ivs[3];
  result.ivs.IV_SPD = ivs[4];
  result.ivs.IV_SPE = ivs[5];
}
