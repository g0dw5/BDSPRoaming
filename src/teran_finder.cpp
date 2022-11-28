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

void TeranFinder::FindAllResult()
{
  bool is_scarlet = true;

  int miniv = 5;
  bool is_black = true;

  ThreadPool thread_pool(1);

  std::list<std::future<Result>> futures;

  for (ulong seed = 0; seed < 0x100000000L; ++seed)
  {
    futures.emplace_back(thread_pool.enqueue(
        [this, is_scarlet, seed, miniv, is_black]
        {
          Result result;
          generate_and_check(is_scarlet, seed, miniv, is_black, result);

          return result;
        }));
  }

  for (auto& f : futures)
  {
    Result result = f.get();
    if (result.ivs.IV_HP != 31 || result.ivs.IV_ATK != 31 ||
        result.ivs.IV_DEF != 31 || result.ivs.IV_SPA != 31 ||
        result.ivs.IV_SPD != 31 || result.ivs.IV_SPE != 31)
    {
      continue;
    }

    result_array_.emplace_back(result);
  }

  for (const auto& result : result_array_)
  {
    std::cout << result.species << "," << result.pid << "," << result.shiny_type
              << "," << result.teran_type << std::endl;
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

void TeranFinder::generate_and_check(bool is_scarlet, uint32_t seed, int miniv,
                                     bool is_black, TeranFinder::Result& result)
{
  // 精灵信息
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
    if (result.shiny_type != 1)
      return;

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
        if (ivs[idx] != kFlawlessValue)
        {
          return;
        }
      }
    }

    result.ivs.IV_HP = ivs[0];
    result.ivs.IV_ATK = ivs[1];
    result.ivs.IV_DEF = ivs[2];
    result.ivs.IV_SPA = ivs[3];
    result.ivs.IV_SPD = ivs[4];
    result.ivs.IV_SPE = ivs[5];
  }

  // 获取太晶类型(18位)
  result.teran_type = Xoroshiro128Plus(seed).NextInt(18);

  // 精灵种族与星级判定
  {
    Xoroshiro128Plus rng(seed);

    uint Difficulty = rng.NextInt(100);
    int start_count = GetStarCount(Difficulty, 4, is_black);

    uint total = is_scarlet ? GetRateTotalBaseScarlet(start_count)
                            : GetRateTotalBaseViolet(start_count);

    int species_roll = rng.NextInt(total);

    auto iter_star = encounter_hash_.find(start_count);
    for (const auto& encounter : iter_star->second)
    {
      int minimum = is_scarlet ? encounter.rand_rate_min_scarlet
                               : encounter.rand_rate_min_violet;

      if (abs(species_roll - minimum) < encounter.rand_rate)
      {
        result.species = encounter.species;
        // TODO(wang.song) 看配置表像是2命中,可以优化性能
        return;
      }
    }
  }
}